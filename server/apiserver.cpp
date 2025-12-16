#include "apiserver.h"
#include <QDebug>
#include <QHostAddress>
#include <QJsonParseError>
#include <QDir>
#include <QTemporaryFile>
#include <QFile>
#include <QDateTime>
#include "emailconfig.h"
#include "database.h"
#include "pdfgenerator.h"
#include "smtpclient.h"
#include "mimepart.h"
#include "mimehtml.h"
#include "mimeattachment.h"
#include "mimemessage.h"
#include "mimetext.h"
#include "mimeinlinefile.h"
#include "mimefile.h"
#include "mimebytearrayattachment.h"

ApiServer::ApiServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_sslEnabled(false)
    , m_maxConnections(100)
    , m_connectionTimeout(300)
    , m_bookingTimeout(15)
{
    m_stats.activeConnections = 0;
    m_stats.totalConnections = 0;
    m_stats.bytesRecieved = 0;
    m_stats.bytesSent = 0;
    m_stats.authenticatedUsers = 0;

    connect(m_server, &QTcpServer::newConnection, this, &ApiServer::onNewConnection);

    m_cleanup_Timer = new QTimer(this);
    connect(m_cleanup_Timer, &QTimer::timeout, this, &ApiServer::onCleanupTimer);
    m_cleanup_Timer->start(60000);
}

ApiServer::~ApiServer(){
    stopServer();
}

bool ApiServer::startServer(quint16 port, const QString &address){
    if (m_server->isListening()){
        qDebug() << "Server is already running";
        return false;
    }

    QHostAddress addr(address);
    if(!m_server->listen(addr, port)){
        QString error = m_server->errorString();
        qDebug() << "Failed to start server:" << error;
        emit errorOccured(error);
        return false;
    }

    m_stats.startTime = QDateTime::currentDateTime();
    qDebug() << "Server started on" << address << ":" << port;
    emit serverStarted(port);
    return true;
}

void ApiServer::stopServer(){
    if (!m_server->isListening()){
        return;
    }

    QMutexLocker locker(&m_mutex);
    for (auto it = m_clients.begin(); it != m_clients.end(); it++){
        it.key()->disconnectFromHost();
        delete it.value();
    }
    m_clients.clear();

    m_server->close();
    qDebug() << "Server stopped";
    emit serverStopped();
}

bool ApiServer::isRunning() const
{
    return m_server->isListening();
}

bool ApiServer::enableSSL(const QString &certPath, const QString &keyPath){
    m_certPath = certPath;
    m_keyPath = keyPath;
    m_sslEnabled = true;
    qDebug() << "SSL enabled with cert:" << certPath;
    return true;
}

bool ApiServer::isSSLEnabled() const{
    return m_sslEnabled;
}

void ApiServer::setMaxConnections(int max){
    m_maxConnections = max;
    m_server->setMaxPendingConnections(max);
}

void ApiServer::setConnectionTimeout(int seconds){
    m_connectionTimeout = seconds;
}

void ApiServer::setBookingTimeout(int minutes){
    m_bookingTimeout = minutes;
}

ApiServer::ServerStats ApiServer::getStatistics() const{
    return m_stats;
}

void ApiServer::onNewConnection(){
    while (m_server->hasPendingConnections()){
        QTcpSocket* socket = m_server->nextPendingConnection();

        if (m_clients.size() >= m_maxConnections){
            qDebug() << "Max connections reached, rejecting" << socket->peerAddress().toString();
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }

        ClientHandler* handler = new ClientHandler(socket, this);
        connect(handler, &ClientHandler::disconnected, this, &ApiServer::onClientDisconnected);
        connect(handler, &ClientHandler::errorOccurred, this, &ApiServer::onClientError);
        connect(handler, &ClientHandler::authenticated, this, &ApiServer::authenticationSuccess);

        QMutexLocker locker(&m_mutex);
        m_clients.insert(socket, handler);
        m_stats.activeConnections = m_clients.size();
        m_stats.totalConnections++;

        QString addr = socket->peerAddress().toString();
        quint16 port = socket->peerPort();

        qDebug() << "Client connected:" << addr << ":" << port;
        emit clientConnected(addr, port);

        Database::instance().logAction(-1, "client_connected", addr, QString("Port: %1").arg(port), true);
    }
}

void ApiServer::onClientDisconnected(){
    ClientHandler* handler = qobject_cast<ClientHandler*>(sender());
    if (!handler) return;

    QMutexLocker locker(&m_mutex);
    for(auto it = m_clients.begin(); it != m_clients.end(); it++){
        if (it.value() == handler){
            QString addr = handler->getAddress();

            if (handler->isAuthenticated()){
                Database::instance().invalidateSession(handler->getSessionToken());
                m_stats.authenticatedUsers--;
            }

            qDebug() <<"Client disconnected:" << addr;
            emit clientDisconnected(addr);

            Database::instance().logAction(handler->getUserId(), "client_disconnected", addr, "", true);
            m_clients.erase(it);
            m_stats.activeConnections = m_clients.size();
            handler->deleteLater();
            break;
        }
    }
}

void ApiServer::onClientError(QAbstractSocket::SocketError error){
    ClientHandler* handler = qobject_cast<ClientHandler*>(sender());
    if (!handler) return;

    QString errorStr = QString("Socket error %1: %2").arg(error).arg(handler->getAddress());
    qWarning() << errorStr;
    emit errorOccured(errorStr);
}

void ApiServer::onCleanupTimer(){
    Database::instance().cleanupExpiredSessions();
    Database::instance().cleanupExpiredBookings();
    Database::instance().cleanupExpiredVerificationCodes();
}

void ApiServer::broadcastMessage(const QJsonObject &message, QTcpSocket *exclude){
    QMutexLocker locker(&m_mutex);
    for (auto it = m_clients.begin(); it != m_clients.end(); it++){
        if (it.key() != exclude && it.value()->isAuthenticated()){
            it.value()->sendResponse(message);
        }
    }
}




ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_authenticated(false)
    , m_userId(-1)
    {
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &ClientHandler::onSocketError);
}

ClientHandler::~ClientHandler(){
    if (m_socket->state() == QAbstractSocket::ConnectedState){
        m_socket->disconnectFromHost();
    }
}

QString ClientHandler::getAddress() const{
    return m_socket->peerAddress().toString();
}

quint16 ClientHandler::getPort() const{
    return m_socket->peerPort();
}

bool ClientHandler::isAuthenticated() const{
    return m_authenticated;
}

int ClientHandler::getUserId() const{
    return m_userId;
}

QString ClientHandler::getSessionToken() const {
    return m_sessionToken;
}

void ClientHandler::sendResponse(const QJsonObject &response){
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    data.append("\n");

    qint64 written = m_socket->write(data);
    m_socket->flush();

    if (written == -1){
        qDebug() << "Failed to send response to" << getAddress();
        qDebug() << "Socket error:" << m_socket->errorString();
    }
}

void ClientHandler::sendError(const QString &error, const QString &command){
    QJsonObject response = createResponse(command, false, error);
    sendResponse(response);
    return;
}

void ClientHandler::onReadyRead(){
    m_buffer.append(m_socket->readAll());

    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1){
        QByteArray message = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);

        if (!message.isEmpty()){
            processMessage(message);
        }
    }

    if (m_buffer.size() > 1024 * 1024){
        qDebug() << "Buffer overflow from" << getAddress();
        m_socket->disconnectFromHost();
    }
}

void ClientHandler::onDisconnected(){
    emit disconnected();
}

void ClientHandler::onSocketError(QAbstractSocket::SocketError error){
    emit errorOccurred(error);
}

void ClientHandler::processMessage(const QByteArray &data){
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError){
        sendError("Invalid JSON: " + parseError.errorString());
    }

    if (!doc.isObject()){
        sendError("Request must be a JSON object");
        return;
    }

    QJsonObject request = doc.object();
    if (!request.contains("command")){
        sendError("Missing 'command' field");
        return;
    }
    handleCommand(request);
}

bool ClientHandler::requireAuth(const QString &command){
    if (!m_authenticated){
        sendError("Authentication required", command);
        return false;
    }
    return true;
}

void ClientHandler::handleCommand(const QJsonObject &request){
    QString command = request["command"].toString();
    QJsonObject data = request["data"].toObject();

    qDebug() << "Command from" << getAddress() << ":" << command;

    if (command == "REGISTER"){
        handleRegister(data);
    }
    else if (command == "LOGIN"){
        handleLogin(data);
    }
    else if (command == "GET_STATIONS"){
        if (requireAuth(command)) handleGetStations(data);
    }
    else if (command == "SEARCH_TRAINS"){
        if (requireAuth(command)) handleSearchTrains(data);
    }
    else if (command == "LOGOUT"){
        if (requireAuth(command)) handleLogout();
    }
    else if (command == "GET_AVAILABLE_SEATS"){
        if (requireAuth(command)) handleGetAvailableSeats(data);
    }
    else if (command == "BOOK_TICKET"){
        if (requireAuth(command)) handleBookTicket(data);
    }
    else if (command == "PAY_TICKET"){
        if (requireAuth(command)) handlePayTicket(data);
    }
    else if (command == "CANCEL_TICKET"){
        if (requireAuth(command)) handleCancelTicket(data);
    }
    else if (command == "GET_MY_TICKETS"){
        if (requireAuth(command)) handleGetMyTickets();
    }
    else if (command == "GET_TICKET_DETAILS"){
        if (requireAuth(command)) handleGetTicketDetails(data);
    }
    else if (command == "CHANGE_PASSWORD"){
        if (requireAuth(command)) handleChangePassword(data);
    }
    else if (command == "GET_PROFILE"){
        if (requireAuth(command)) handleGetProfile();
    }
    else if (command == "VERIFY_EMAIL"){
        handleVerifyEmail(data);
    }
    else if (command == "RESEND_VERIFICATION"){
        handleResendVerification(data);
    }
    else {
        sendError("Unknown command: " + command, command);
    }

    emit commandExecuted(command, true);
}

QJsonObject ClientHandler::createResponse(const QString& command, bool success, const QString& message, const QJsonObject& data){
    QJsonObject response;
    response["command"] = command;
    response["success"] = success;
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (!message.isEmpty()){
        response["message"] = message;
    }

    if (!data.isEmpty()){
        response["data"] = data;
    }

    return response;
}

QJsonObject ClientHandler::userToJson(const User &user){
    QJsonObject obj;
    obj["id"] = user.id;
    obj["name"] = user.name;
    obj["surname"] = user.surname;
    obj["email"] = user.email;
    obj["createdAt"] = user.createdAt.toString(Qt::ISODate);
    obj["isVerified"] = user.isVerified;
    if (user.lastLogin.isValid()){
        obj["lastLogin"] = user.lastLogin.toString(Qt::ISODate);
    }
    return obj;
}

QJsonObject ClientHandler::stationToJson(const Station& station)
{
    QJsonObject obj;
    obj["id"] = station.id;
    obj["name"] = station.name;
    obj["city"] = station.city;
    obj["code"] = station.code;
    obj["latitude"] = station.latitude;
    obj["longitude"] = station.longitude;
    return obj;
}

QJsonObject ClientHandler::ticketToJson(const Ticket& ticket)
{
    QJsonObject obj;
    obj["id"] = ticket.id;
    obj["ticketNumber"] = ticket.ticketNumber;
    obj["scheduleId"] = ticket.scheduleId;
    obj["seatId"] = ticket.seatId;
    obj["departureStationId"] = ticket.departureStationId;
    obj["arrivalStationId"] = ticket.arrivalStationId;
    obj["price"] = ticket.price;
    obj["status"] = ticket.status;
    obj["passengerName"] = ticket.passengerName;
    obj["passengerDocument"] = ticket.passengerDocument;
    obj["bookedAt"] = ticket.bookedAt.toString(Qt::ISODate);

    if (ticket.paidAt.isValid()){
        obj["paidAt"] = ticket.paidAt.toString(Qt::ISODate);
    }
    if (ticket.cancelledAt.isValid()){
        obj["cancelledAt"] = ticket.cancelledAt.toString(Qt::ISODate);
    }

    return obj;
}

QJsonObject ClientHandler::searchResultToJson(const Database::SearchResult& result)
{
    QJsonObject obj;
    obj["scheduleId"] = result.scheduleId;
    obj["routeId"] = result.routeId;
    obj["trainNumber"] = result.trainNumber;
    obj["trainType"] = result.trainType;
    obj["departureStationId"] = result.departureStationId;
    obj["departureStationName"] = result.departureStationName;
    obj["arrivalStationId"] = result.arrivalStationId;
    obj["arrivalStationName"] = result.arrivalStationName;
    obj["departureTime"] = result.departureTime.toString(Qt::ISODate);
    obj["arrivalTime"] = result.arrivalTime.toString(Qt::ISODate);
    obj["travelTimeMinutes"] = result.travelTimeMinutes;
    obj["minPrice"] = result.minPrice;
    obj["availableSeats"] = result.availableSeats;
    return obj;
}

void ClientHandler::handleRegister(const QJsonObject &data){
    QString name = data["name"].toString();
    QString surname = data["surname"].toString();
    QString email = data["email"].toString();
    QString password = data["password"].toString();

    if (name.isEmpty() || surname.isEmpty() || email.isEmpty() || password.isEmpty()){
        sendError("All fields are required", "REGISTER");
        return;
    }

    int userId;
    bool success = Database::instance().createUser(name, surname, email, password, &userId);
    if (success){
        QString verificationCode = Database::instance().createVerificationCode(userId, email);

        if (verificationCode.isEmpty()) {
            sendError("Failed to create verification code", "REGISTER");
            return;
        }

        QJsonObject responseData;
        responseData["userId"] = userId;
        responseData["email"] = email;
        responseData["requiresVerification"] = true;

        sendResponse(createResponse("REGISTER", true, "Registration successful. Please check your email for verification code.", responseData));

        qDebug() << "User registered:" << email << "ID:" << userId;
        sendVerificationEmail(email, verificationCode);
    } else{
        sendError(Database::instance().lastError(), "REGISTER");
    }
}

void ClientHandler::handleLogin(const QJsonObject &data){
    QString email = data["email"].toString();
    QString password = data["password"].toString();

    if (email.isEmpty() || password.isEmpty()){
        sendError("Email and password are required", "LOGIN");
        return;
    }

    bool found;
    User user = Database::instance().getUserByEmail(email, &found);

    if (!found){
        sendError("Invalid email or password", "LOGIN");
        return;
    }

    if (!user.isVerified){
        sendError("Please verify your email first", "LOGIN");
        return;
    }

    QString errorMsg;
    bool success = Database::instance().checkPassword(email, password, &errorMsg);

    if (!success){
        sendError(errorMsg, "LOGIN");
        Database::instance().logAction(-1, "login_failed", getAddress(), email, false);
        return;
    }

    m_sessionToken = Database::instance().createSession(user.id, getAddress(), "ApiClient");

    if (m_sessionToken.isEmpty()){
        sendError("Failed to create session", "LOGIN");
        return;
    }

    Database::instance().updateLastLogin(email, getAddress());
    m_authenticated = true;
    m_userId = user.id;
    m_userEmail = email;

    QJsonObject responseData;
    responseData["sessionToken"] = m_sessionToken;
    responseData["user"] = userToJson(user);

    sendResponse(createResponse("LOGIN", true, "Login successful", responseData));

    qDebug() << "User logged in:" << email;
    emit authenticated(user.id, email);
}

void ClientHandler::handleLogout()
{
    if (m_authenticated){
        Database::instance().invalidateSession(m_sessionToken);
        Database::instance().logAction(m_userId, "logout", getAddress(), "", true);

        qDebug() << "User logged out:" << m_userEmail;

        m_authenticated = false;
        m_userId = -1;
        m_userEmail.clear();
        m_sessionToken.clear();

        sendResponse(createResponse("LOGOUT", true, "Logout successful"));
    }
}

void ClientHandler::handleGetStations(const QJsonObject &data){
    QString search = data["search"].toString();

    QList<Station> stations;
    if (search.isEmpty()){
        stations = Database::instance().getAllStations();
    } else{
        stations = Database::instance().searchStations(search);
    }

    QJsonArray stationsArray;
    for (const Station& station: stations){
        stationsArray.append(stationToJson(station));
    }

    QJsonObject responseData;
    responseData["stations"] = stationsArray;
    responseData["count"] = stations.size();

    sendResponse(createResponse("GET_STATIONS", true, "", responseData));
}

void ClientHandler::handleSearchTrains(const QJsonObject &data){
    int departureStationId = data["departureStationId"].toInt();
    int arrivalStationId = data["arrivalStationId"].toInt();
    QString dateStr = data["date"].toString();

    if (departureStationId <= 0 || arrivalStationId <= 0 || dateStr.isEmpty()){
        sendError("Invalid search parameters", "SEARCH_TRAINS");
        return;
    }

    QDate date = QDate::fromString(dateStr, Qt::ISODate);
    if (!date.isValid()){
        sendError("Invalid date format (use YYYY-MM-DD)", "SEARCH_TRAINS");
        return;
    }

    QList<Database::SearchResult> results = Database::instance().searchTrains(departureStationId, arrivalStationId, date);

    QJsonArray resultsArray;
    for (const auto& result: results){
        resultsArray.append(searchResultToJson(result));
    }

    QJsonObject responseData;
    responseData["trains"] = resultsArray;
    responseData["count"] = results.size();

    sendResponse(createResponse("SEARCH_TRAINS", true, "", responseData));

    qDebug() << "Search:" << departureStationId << "->" << arrivalStationId
             << "on" << dateStr << "found" << results.size() << "trains";
}

void ClientHandler::handleGetAvailableSeats(const QJsonObject &data){
    int scheduleId = data["scheduleId"].toInt();
    int departureStationId = data["departureStationId"].toInt();
    int arrivalStationId = data["arrivalStationId"].toInt();

    if (scheduleId <= 0 || departureStationId <= 0 || arrivalStationId <= 0){
        sendError("Invalid parameters", "GET_AVAILABLE_SEATS");
        return;
    }

    QList<Seat> seats = Database::instance().getAvailableSeats(scheduleId, departureStationId, arrivalStationId);

    QJsonArray seatsArray;
    for (const Seat& seat: seats){
        QJsonObject seatObj;
        seatObj["id"] = seat.id;
        seatObj["carriageId"] = seat.carriageId;
        seatObj["carriageNumber"] = seat.carriageNumber;
        seatObj["carriageType"] = seat.carriageType;
        seatObj["seatNumber"] = seat.seatNumber;
        seatObj["seatType"] = seat.seatType;
        seatObj["isAvailable"] = seat.isAvailable;
        seatsArray.append(seatObj);
    }

    QJsonObject responseData;
    responseData["seats"] = seatsArray;
    responseData["count"] = seats.size();

    sendResponse(createResponse("GET_AVAILABLE_SEATS", true, "", responseData));
}

void ClientHandler::handleBookTicket(const QJsonObject &data){
    int scheduleId = data["scheduleId"].toInt();
    int seatId = data["seatId"].toInt();
    int departureStationId = data["departureStationId"].toInt();
    int arrivalStationId = data["arrivalStationId"].toInt();
    QString passengerName = data["passengerName"].toString();
    QString passengerDocument = data["passengerDocument"].toString();
    double price = data["price"].toDouble();

    if (scheduleId <= 0 || seatId <= 0 || departureStationId <= 0 ||
        arrivalStationId <= 0 || passengerName.isEmpty() ||
        passengerDocument.isEmpty() || price <= 0){
        sendError("Invalid booking parameters", "BOOK_TICKET");
        return;
    }

    QString ticketNumber = Database::instance().bookTicket(
        m_userId, scheduleId, seatId, departureStationId, arrivalStationId,
        passengerName, passengerDocument, price);

    if (ticketNumber.isEmpty()){
        sendError(Database::instance().lastError(), "BOOK_TICKET");
        return;
    }

    QJsonObject responseData;
    responseData["ticketNumber"] = ticketNumber;
    responseData["status"] = "booked";
    responseData["message"] = "Билет успешно забронирован. Пожалуйста, оплатите его в течение 15 минут.";

    sendResponse(createResponse("BOOK_TICKET", true, "", responseData));

    qDebug() << "Ticket booked:" << ticketNumber << "by user" << m_userId;
}

void ClientHandler::handlePayTicket(const QJsonObject& data){
    QString ticketNumber = data["ticketNumber"].toString();

    if (ticketNumber.isEmpty()){
        sendError("Ticket number is required", "PAY_TICKET");
        return;
    }

    bool found;
    Ticket ticket = Database::instance().getTicket(ticketNumber, &found);

    if (!found){
        sendError("Ticket not found", "PAY_TICKET");
        return;
    }

    if (ticket.userId != m_userId){
        sendError("Access denied", "PAY_TICKET");
        return;
    }

    if (ticket.status != "booked"){
        sendError("Ticket cannot be paid (status: " + ticket.status + ")", "PAY_TICKET");
        return;
    }

    bool success = Database::instance().payTicket(ticketNumber);

    if (success){
        QJsonObject responseData;
        responseData["ticketNumber"] = ticketNumber;
        responseData["status"] = "paid";

        sendResponse(createResponse("PAY_TICKET", true, "Payment successful", responseData));

        Database::instance().logAction(m_userId, "ticket_paid", getAddress(), ticketNumber, true);

        TicketFullInfo ticketInfo = Database::instance().getTicketFullInfo(ticketNumber, &found);

        if (found) {
            QByteArray pdfData = PdfGenerator::generateTicketPdf(
                ticketInfo.ticket,
                ticketInfo.trainNumber,
                ticketInfo.trainType,
                ticketInfo.departureStationName,
                ticketInfo.arrivalStationName,
                ticketInfo.departureTime,
                ticketInfo.arrivalTime,
                ticketInfo.carriageNumber,
                ticketInfo.seatNumber
                );
            if (!pdfData.isEmpty()) {
                sendTicketEmail(m_userEmail, ticketInfo.ticket, pdfData);
                qDebug() << "PDF ticket sent to" << m_userEmail;
            } else {
                qWarning() << "Failed to generate PDF for ticket:" << ticketNumber;
            }
        } else {
            qWarning() << "Failed to get full ticket info for:" << ticketNumber;
        }

    } else {
        sendError(Database::instance().lastError(), "PAY_TICKET");
    }
}

void ClientHandler::sendTicketEmail(const QString& recipientEmail, const Ticket& ticket, const QByteArray& pdfData){
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/ticket_XXXXXX.pdf");
    if (!tempFile.open()) {
        qWarning() << "Failed to create temporary file for PDF attachment.";
        return;
    }

    tempFile.write(pdfData);
    tempFile.close();

    QFile* attachmentFile = new QFile(tempFile.fileName());
    if (!attachmentFile->open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open temporary file for MimeAttachment:" << attachmentFile->errorString();
        delete attachmentFile;
        return;
    }

    MimeAttachment attachment(attachmentFile, QString("Ticket_%1.pdf").arg(ticket.ticketNumber));
    attachment.setContentType("application/pdf");

    MimeMessage message;
    message.setSender(EmailAddress("a.pechenkin3@gmail.com", "Railway Booking System"));
    message.addRecipient(EmailAddress(recipientEmail));
    message.setSubject(QString("Билет %1 - Оплачен").arg(ticket.ticketNumber));

    MimeText text;
    text.setText(QString(
                     "Здравствуйте!\n\n"
                     "Ваш билет успешно оплачен.\n\n"
                     "Номер билета: %1\n"
                     "Пассажир: %2\n"
                     "Стоимость: %3 ₽\n\n"
                     "Электронный билет во вложении.\n\n"
                     "Приятного путешествия!\n\n"
                     "---\n"
                     "Система бронирования железнодорожных билетов"
                     ).arg(ticket.ticketNumber)
                     .arg(ticket.passengerName)
                     .arg(ticket.price, 0, 'f', 2));

    message.addPart(&text);
    message.addPart(&attachment);

    bool isLoaded = EmailConfig::instance().loadConfig();
    if (!isLoaded) {
        qDebug() << "Не удалось загрузить конфигурацию email";
        return;
    }

    SmtpClient smtp(EmailConfig::instance().smtpHost(),
                    EmailConfig::instance().smtpPort(),
                    SmtpClient::SslConnection);

    smtp.connectToHost();
    if (!smtp.waitForReadyConnected()) {
        qDebug() << "Failed to connect to SMTP host!";
        return;
    }

    smtp.login(EmailConfig::instance().username(), EmailConfig::instance().password());
    if (!smtp.waitForAuthenticated()) {
        qDebug() << "Failed to authenticate!";
        return;
    }

    smtp.sendMail(message);
    if (!smtp.waitForMailSent()) {
        qDebug() << "Failed to send email!";
        return;
    }

    qDebug() << "Ticket PDF sent to" << recipientEmail;
    smtp.quit();
}

void ClientHandler::handleCancelTicket(const QJsonObject& data){
    QString ticketNumber = data["ticketNumber"].toString();
    QString reason = data["reason"].toString("User cancellation");

    if (ticketNumber.isEmpty()){
        sendError("Ticket number is required", "CANCEL_TICKET");
        return;
    }

    bool found;
    Ticket ticket = Database::instance().getTicket(ticketNumber, &found);

    if (!found){
        sendError("Ticket not found", "CANCEL_TICKET");
        return;
    }

    if (ticket.userId != m_userId){
        sendError("Access denied", "CANCEL_TICKET");
        return;
    }

    bool success = Database::instance().cancelTicket(ticketNumber, reason);

    if (success){
        QJsonObject responseData;
        responseData["ticketNumber"] = ticketNumber;
        responseData["status"] = "cancelled";

        sendResponse(createResponse("CANCEL_TICKET", true, "Ticket cancelled", responseData));

        Database::instance().logAction(m_userId, "ticket_cancelled", getAddress(), ticketNumber, true);
    } else {
        sendError(Database::instance().lastError(), "CANCEL_TICKET");
    }
}

void ClientHandler::handleGetMyTickets()
{
    QList<Ticket> tickets = Database::instance().getUserTickets(m_userId);

    QJsonArray ticketsArray;
    for (const Ticket& ticket: tickets){
        ticketsArray.append(ticketToJson(ticket));
    }

    QJsonObject responseData;
    responseData["tickets"] = ticketsArray;
    responseData["count"] = tickets.size();

    sendResponse(createResponse("GET_MY_TICKETS", true, "", responseData));
}

void ClientHandler::handleGetTicketDetails(const QJsonObject& data)
{
    QString ticketNumber = data["ticketNumber"].toString();

    if (ticketNumber.isEmpty()){
        sendError("Ticket number is required", "GET_TICKET_DETAILS");
        return;
    }

    bool found;
    Ticket ticket = Database::instance().getTicket(ticketNumber, &found);

    if (!found){
        sendError("Ticket not found", "GET_TICKET_DETAILS");
        return;
    }

    if (ticket.userId != m_userId){
        sendError("Access denied", "GET_TICKET_DETAILS");
        return;
    }

    QJsonObject responseData;
    responseData["ticket"] = ticketToJson(ticket);

    sendResponse(createResponse("GET_TICKET_DETAILS", true, "", responseData));
}

void ClientHandler::handleChangePassword(const QJsonObject& data)
{
    QString oldPassword = data["oldPassword"].toString();
    QString newPassword = data["newPassword"].toString();

    if (oldPassword.isEmpty() || newPassword.isEmpty()){
        sendError("Both old and new password are required", "CHANGE_PASSWORD");
        return;
    }

    bool success = Database::instance().changePassword(
        m_userEmail, oldPassword, newPassword);

    if (success){
        sendResponse(createResponse("CHANGE_PASSWORD", true, "Password changed successfully"));

        Database::instance().logAction(m_userId, "password_changed", getAddress(), "", true);
    }else {
        sendError(Database::instance().lastError(), "CHANGE_PASSWORD");
    }
}

void ClientHandler::handleGetProfile()
{
    bool found;
    User user = Database::instance().getUserById(m_userId, &found);

    if (!found){
        sendError("User not found", "GET_PROFILE");
        return;
    }

    QJsonObject responseData;
    responseData["user"] = userToJson(user);

    sendResponse(createResponse("GET_PROFILE", true, "", responseData));
}

void ClientHandler::handleResendVerification(const QJsonObject& data){
    QString email = data["email"].toString();

    if (email.isEmpty()){
        sendError("Email is required", "RESEND_VERIFICATION");
        return;
    }

    bool found;
    User user = Database::instance().getUserByEmail(email, &found);

    if (!found){
        sendError("User not found", "RESEND_VERIFICATION");
        return;
    }

    if (user.isVerified){
        sendError("Email already verified", "RESEND_VERIFICATION");
        return;
    }

    QString code = Database::instance().createVerificationCode(user.id, email);

    if (code.isEmpty()){
        sendError("Failed to create verification code", "RESEND_VERIFICATION");
        return;
    }

    QJsonObject responseData;
    responseData["message"] = "Verification code sent to email";
    sendResponse(createResponse("RESEND_VERIFICATION", true, "Verification code has been sent to your email", responseData));

    sendVerificationEmail(email, code);
}

void ClientHandler::sendVerificationEmail(const QString& recipientEmail, const QString& code){
    MimeMessage message;
    message.setSender(EmailAddress("a.pechenkin3@gmail.com", "Служба поддержки"));
    message.addRecipient(EmailAddress(recipientEmail));
    message.setSubject("Код подтверждения для регистрации");

    MimeText text;
    text.setText(QString("Ваш код подтверждения: %1\n"
                         "Пожалуйста, введите этот код в форму регистрации.\n"
                         "Код действителен в течение 15 минут.\n\n"
                         "Если вы не регистрировались в нашем приложении, проигнорируйте это письмо.")
                     .arg(code));
    message.addPart(&text);

    bool isLoaded = EmailConfig::instance().loadConfig();
    if (!isLoaded) {
        qDebug() << "Не удалось загрузить конфигурацию email";
        return;
    }

    SmtpClient smtp(EmailConfig::instance().smtpHost(),
                    EmailConfig::instance().smtpPort(),
                    SmtpClient::SslConnection);

    smtp.connectToHost();
    if (!smtp.waitForReadyConnected()) {
        qDebug() << "Failed to connect to host!";
        return;
    }

    smtp.login(EmailConfig::instance().username(), EmailConfig::instance().password());
    if (!smtp.waitForAuthenticated()) {
        qDebug() << "Failed to login!";
        return;
    }

    smtp.sendMail(message);
    if (!smtp.waitForMailSent()) {
        qDebug() << "Failed to send mail!";
        return;
    }

    qDebug() << "Письмо успешно отправлено на" << recipientEmail;
    smtp.quit();
}

void ClientHandler::handleVerifyEmail(const QJsonObject& data){
    QString email = data["email"].toString();
    QString code = data["code"].toString();

    if (email.isEmpty() || code.isEmpty()){
        sendError("Email and code are required", "VERIFY_EMAIL");
        return;
    }

    QString errorMsg;
    bool success = Database::instance().verifyEmail(email, code, &errorMsg);

    if (success){
        QJsonObject responseData;
        responseData["email"] = email;
        responseData["verified"] = true;

        sendResponse(createResponse("VERIFY_EMAIL", true, "Email verified successfully", responseData));

        qDebug() << "Email verified:" << email;
    } else {
        sendError(errorMsg, "VERIFY_EMAIL");
    }
}
















