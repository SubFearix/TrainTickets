#include "apiclient.h"
#include "config.h"
#include <QDebug>
#include <QUrl>

ApiClient::ApiClient() : QObject(nullptr)
    , m_socket(new QTcpSocket(this))
    , m_authenticated(false){

    connect(m_socket, &QTcpSocket::connected, this, &ApiClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ApiClient:: onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ApiClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>:: of(&QTcpSocket::errorOccurred), this, &ApiClient:: onSocketError);
}

void ApiClient::connectToServer(const QString& host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Already connected";
        return;
    }

    qDebug() << "Connecting to" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

void ApiClient::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool ApiClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void ApiClient::onConnected()
{
    qDebug() << "Connected to server";
    emit connected();
}

void ApiClient::onDisconnected()
{
    qDebug() << "Disconnected from server";
    m_authenticated = false;
    m_sessionToken.clear();
    emit disconnected();
}

void ApiClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1) {
        QByteArray message = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);

        if (!message.isEmpty()) {
            processResponse(message);
        }
    }
}

void ApiClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString errorMsg = QString("Ошибка сети: %1").arg(m_socket->errorString());
    qWarning() << errorMsg;
    emit error(errorMsg);
}


void ApiClient::sendCommand(const QString &command, const QJsonObject &data){
    if (!isConnected()) {
        emit error("Нет подключения к серверу");
        return;
    }

    QJsonObject request;
    request["command"] = command;

    if (!data.isEmpty()) {
        request["data"] = data;
    }

    QJsonDocument doc(request);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    postData.append("\n");

    qDebug() << "Sending command:" << command;
    qDebug() << "Request:" << postData;

    m_socket->write(postData);
    m_socket->flush();
}

void ApiClient::processResponse(const QByteArray &data){
    qDebug() << "=== CLIENT: RAW RESPONSE ===" << data;//

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonObject response = doc.object();
    QString command = response["command"].toString();
    bool success = response["success"].toBool();
    QString message = response["message"].toString();

    if (command != "GET_AVAILABLE_SEATS") {
        qDebug() << "Response:" << command << "Success:" << success;
    }

    if (!success) {
        if (command == "REGISTER") {
            emit registerFailed(message);
        } else if (command == "LOGIN") {
            emit loginFailed(message);
        } else if (command == "VERIFY_EMAIL") {
            emit verificationFailed(message);
        } else if (command == "CHANGE_PASSWORD") {
            emit passwordChangeError(message);
        } else {
            emit error(QString("%1: %2").arg(command, message));
        }
        return;
    }

    if (command == "REGISTER") {
        handleRegisterResponse(response);
    } else if (command == "LOGIN") {
        handleLoginResponse(response);
    } else if (command == "LOGOUT") {
        handleLogoutResponse(response);
    } else if (command == "VERIFY_EMAIL") {
        handleVerifyEmailResponse(response);
    } else if (command == "GET_STATIONS") {
        handleStationsResponse(response);
    } else if (command == "SEARCH_TRAINS") {
        handleTrainsResponse(response);
    } else if (command == "GET_AVAILABLE_SEATS") {
        handleSeatsResponse(response);
    } else if (command == "BOOK_TICKET") {
        handleBookTicketResponse(response);
    } else if (command == "PAY_TICKET") {
        handlePayTicketResponse(response);
    } else if (command == "RESEND_VERIFICATION") {
        emit resendVerificationSuccess();
    } else if (command == "CANCEL_TICKET") {
        handleCancelTicketResponse(response);
    } else if (command == "GET_MY_TICKETS") {
        handleMyTicketsResponse(response);
    } else if (command == "GET_TICKET_DETAILS") {
        handleTicketDetailsResponse(response);
    } else if (command == "GET_PROFILE") {
        handleProfileResponse(response);
    } else if (command == "CHANGE_PASSWORD") {
        handlePasswordChangeResponse(response);
    }
}

void ApiClient::registerUser(const QString &name, const QString &surname, const QString &email, const QString &password){
    QJsonObject data;
    data["name"] = name;
    data["surname"] = surname;
    data["email"] = email;
    data["password"] = password;

    sendCommand("REGISTER", data);
}

void ApiClient::login(const QString &email, const QString &password){
    QJsonObject data;
    data["email"] = email;
    data["password"] = password;

    sendCommand("LOGIN", data);
}

void ApiClient::logout(){
    sendCommand("LOGOUT");
}

void ApiClient::verifyEmail(const QString& email, const QString& code)
{
    QJsonObject data;
    data["email"] = email;
    data["code"] = code;

    sendCommand("VERIFY_EMAIL", data);
}

void ApiClient::resendVerification(const QString& email)
{
    QJsonObject data;
    data["email"] = email;

    sendCommand("RESEND_VERIFICATION", data);
}

void ApiClient::getProfile()
{
    sendCommand("GET_PROFILE");
}

void ApiClient::changePassword(const QString& oldPassword, const QString& newPassword)
{
    QJsonObject data;
    data["oldPassword"] = oldPassword;
    data["newPassword"] = newPassword;

    sendCommand("CHANGE_PASSWORD", data);
}

void ApiClient::getStations(const QString& search)
{
    QJsonObject data;
    if (!search.isEmpty()) {
        data["search"] = search;
    }

    sendCommand("GET_STATIONS", data);
}

void ApiClient::searchTrains(int departureStationId, int arrivalStationId, const QDate& date)
{
    QJsonObject data;
    data["departureStationId"] = departureStationId;
    data["arrivalStationId"] = arrivalStationId;
    data["date"] = date.toString(Qt::ISODate);

    sendCommand("SEARCH_TRAINS", data);
}

void ApiClient::getAvailableSeats(int scheduleId, int departureStationId, int arrivalStationId)
{
    QJsonObject data;
    data["scheduleId"] = scheduleId;
    data["departureStationId"] = departureStationId;
    data["arrivalStationId"] = arrivalStationId;

    sendCommand("GET_AVAILABLE_SEATS", data);
}

void ApiClient::bookTicket(int scheduleId, int seatId, int departureStationId, int arrivalStationId, const QString &passengerName, const QString &passengerDocument, double price){
    QJsonObject data;
    data["scheduleId"] = scheduleId;
    data["seatId"] = seatId;
    data["departureStationId"] = departureStationId;
    data["arrivalStationId"] = arrivalStationId;
    data["passengerName"] = passengerName;
    data["passengerDocument"] = passengerDocument;
    data["price"] = price;

    sendCommand("BOOK_TICKET", data);
}

void ApiClient::payTicket(const QString& ticketNumber)
{
    QJsonObject data;
    data["ticketNumber"] = ticketNumber;

    sendCommand("PAY_TICKET", data);
}

void ApiClient::cancelTicket(const QString& ticketNumber, const QString& reason)
{
    QJsonObject data;
    data["ticketNumber"] = ticketNumber;
    if (!reason.isEmpty()) {
        data["reason"] = reason;
    }

    sendCommand("CANCEL_TICKET", data);
}

void ApiClient::getMyTickets()
{
    sendCommand("GET_MY_TICKETS");
}

void ApiClient::getTicketDetails(const QString& ticketNumber)
{
    QJsonObject data;
    data["ticketNumber"] = ticketNumber;

    sendCommand("GET_TICKET_DETAILS", data);
}

void ApiClient::handleRegisterResponse(const QJsonObject &response){
    QJsonObject data = response["data"].toObject();
    int userId = data["userId"].toInt();
    QString email = data["email"].toString();
    bool requiresVerification = data["requiresVerification"].toBool();

    emit registerSuccess(userId, email, requiresVerification);
}

void ApiClient::handleLoginResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    m_sessionToken = data["sessionToken"].toString();
    m_authenticated = true;

    QJsonObject userObj = data["user"].toObject();
    m_userProfile.id = userObj["id"].toInt();
    m_userProfile.name = userObj["name"].toString();
    m_userProfile.surname = userObj["surname"].toString();
    m_userProfile.email = userObj["email"].toString();
    m_userProfile.createdAt = QDateTime::fromString(userObj["createdAt"].toString(), Qt::ISODate);
    m_userProfile.isVerified = userObj["isVerified"].toBool();

    if (userObj.contains("lastLogin") && !userObj["lastLogin"].isNull()) {
        m_userProfile.lastLogin = QDateTime::fromString(userObj["lastLogin"].toString(), Qt::ISODate);
    }

    emit loginSuccess(m_userProfile);
}

void ApiClient::handleLogoutResponse(const QJsonObject&)
{
    m_authenticated = false;
    m_sessionToken.clear();
    m_userProfile = UserProfile();

    emit logoutSuccess();
}

void ApiClient::handleVerifyEmailResponse(const QJsonObject&)
{
    emit verificationSuccess();
}

void ApiClient::handleStationsResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QJsonArray stationsArray = data["stations"].toArray();

    QList<Station> stations;
    for (const QJsonValue& value: stationsArray) {
        QJsonObject obj = value.toObject();

        Station station;
        station.id = obj["id"].toInt();
        station.name = obj["name"].toString();
        station.city = obj["city"].toString();
        station.code = obj["code"].toString();
        station.latitude = obj["latitude"].toDouble();
        station.longitude = obj["longitude"].toDouble();

        stations.append(station);
    }

    emit stationsReceived(stations);
}

void ApiClient::handleTrainsResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QJsonArray trainsArray = data["trains"].toArray();

    QList<TrainSearchResult> trains;
    for (const QJsonValue& value : trainsArray) {
        QJsonObject obj = value.toObject();

        TrainSearchResult train;
        train.scheduleId = obj["scheduleId"].toInt();
        train.routeId = obj["routeId"].toInt();
        train.trainNumber = obj["trainNumber"].toString();
        train.trainType = obj["trainType"].toString();
        train.departureStationId = obj["departureStationId"].toInt();
        train.departureStationName = obj["departureStationName"].toString();
        train.arrivalStationId = obj["arrivalStationId"].toInt();
        train.arrivalStationName = obj["arrivalStationName"].toString();
        train.departureTime = QDateTime::fromString(obj["departureTime"].toString(), Qt::ISODate);
        train.arrivalTime = QDateTime::fromString(obj["arrivalTime"].toString(), Qt::ISODate);
        train.travelTimeMinutes = obj["travelTimeMinutes"].toInt();
        train.minPrice = obj["minPrice"].toDouble();
        train.availableSeats = obj["availableSeats"].toInt();

        trains.append(train);
    }

    emit trainsReceived(trains);
}

void ApiClient::handleSeatsResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QJsonArray seatsArray = data["seats"].toArray();

    QList<Seat> seats;
    for (const QJsonValue& value: seatsArray) {
        QJsonObject obj = value.toObject();

        Seat seat;
        seat.id = obj["id"].toInt();
        seat.carriageId = obj["carriageId"].toInt();
        seat.carriageNumber = obj["carriageNumber"].toInt();
        seat.carriageType = obj["carriageType"].toString();
        seat.seatNumber = obj["seatNumber"].toInt();
        seat.seatType = obj["seatType"].toString();
        seat.isAvailable = obj["isAvailable"].toBool();

        seats.append(seat);
    }

    emit seatsReceived(seats);
}

void ApiClient::handleBookTicketResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QString ticketNumber = data["ticketNumber"].toString();
    QString status = data["status"].toString();

    emit ticketBooked(ticketNumber, status);
}

void ApiClient::handlePayTicketResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QString ticketNumber = data["ticketNumber"].toString();

    emit ticketPaid(ticketNumber);
}

void ApiClient::handleCancelTicketResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QString ticketNumber = data["ticketNumber"].toString();

    emit ticketCancelled(ticketNumber);
}

void ApiClient::handleMyTicketsResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QJsonArray ticketsArray = data["tickets"].toArray();

    QList<Ticket> tickets;
    for (const QJsonValue& value : ticketsArray) {
        QJsonObject obj = value.toObject();

        Ticket ticket;
        ticket.id = obj["id"].toInt();
        ticket.ticketNumber = obj["ticketNumber"].toString();
        ticket.scheduleId = obj["scheduleId"].toInt();
        ticket.seatId = obj["seatId"].toInt();
        ticket.departureStationId = obj["departureStationId"].toInt();
        ticket.arrivalStationId = obj["arrivalStationId"].toInt();
        ticket.price = obj["price"].toDouble();
        ticket.status = obj["status"].toString();
        ticket.passengerName = obj["passengerName"].toString();
        ticket.passengerDocument = obj["passengerDocument"].toString();
        ticket.bookedAt = QDateTime::fromString(obj["bookedAt"].toString(), Qt::ISODate);

        if (obj.contains("paidAt") && !obj["paidAt"].isNull()) {
            ticket.paidAt = QDateTime::fromString(obj["paidAt"].toString(), Qt::ISODate);
        }
        if (obj.contains("cancelledAt") && !obj["cancelledAt"].isNull()) {
            ticket.cancelledAt = QDateTime::fromString(obj["cancelledAt"].toString(), Qt::ISODate);
        }

        tickets.append(ticket);
    }

    emit ticketsReceived(tickets);
}

void ApiClient::handleTicketDetailsResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QJsonObject obj = data["ticket"].toObject();

    Ticket ticket;
    ticket.id = obj["id"].toInt();
    ticket.ticketNumber = obj["ticketNumber"].toString();
    ticket.scheduleId = obj["scheduleId"].toInt();
    ticket.seatId = obj["seatId"].toInt();
    ticket.departureStationId = obj["departureStationId"].toInt();
    ticket.arrivalStationId = obj["arrivalStationId"].toInt();
    ticket.price = obj["price"].toDouble();
    ticket.status = obj["status"].toString();
    ticket.passengerName = obj["passengerName"].toString();
    ticket.passengerDocument = obj["passengerDocument"].toString();
    ticket.bookedAt = QDateTime::fromString(obj["bookedAt"].toString(), Qt::ISODate);

    if (obj.contains("paidAt") && !obj["paidAt"].isNull()) {
        ticket.paidAt = QDateTime::fromString(obj["paidAt"].toString(), Qt::ISODate);
    }
    if (obj.contains("cancelledAt") && !obj["cancelledAt"].isNull()) {
        ticket.cancelledAt = QDateTime::fromString(obj["cancelledAt"].toString(), Qt::ISODate);
    }

    emit ticketDetailsReceived(ticket);
}

void ApiClient::handleProfileResponse(const QJsonObject& response)
{
    QJsonObject data = response["data"].toObject();
    QJsonObject userObj = data["user"].toObject();

    UserProfile profile;
    profile.id = userObj["id"].toInt();
    profile.name = userObj["name"].toString();
    profile.surname = userObj["surname"].toString();
    profile.email = userObj["email"].toString();
    profile.createdAt = QDateTime::fromString(userObj["createdAt"].toString(), Qt::ISODate);
    profile.isVerified = userObj["isVerified"].toBool();

    if (userObj.contains("lastLogin") && !userObj["lastLogin"].isNull()) {
        profile.lastLogin = QDateTime::fromString(userObj["lastLogin"].toString(), Qt::ISODate);
    }

    emit profileReceived(profile);
}

void ApiClient::handlePasswordChangeResponse(const QJsonObject&)
{
    emit passwordChanged();
}
