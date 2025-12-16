#include "database.h"
#include <QDebug>
#include <QSettings>
#include <QSqlRecord>
#include <QFile>
#include <QVariant>
#include <QRegularExpression>
#include <QUuid>

Database::Database(){
    m_db = QSqlDatabase::addDatabase("QPSQL");
}

Database::~Database(){
    disconnect();
}

Database& Database::instance(){
    static Database instance;
    return instance;
}

bool Database::connect(const QString& host, int port, const QString& dbName, const QString& username, const QString& password){
    QMutexLocker locker(&m_mutex);

    if (m_db.isOpen()){
        qDebug() << "Database is already connected";
        return true;
    }

    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(username);
    m_db.setPassword(password);
    m_db.setConnectOptions("connect_timeout=10;sslmode=prefer");

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qDebug() << "Error while connecting to Database:" << m_lastError;
        emit connectionError(m_lastError);
        return false;
    }
    qDebug() << "Succesfull connection to PostgreSQL";

    if (!initializeTables()){
        qDebug() << "Warning: failed to initialize tables";
    }
    return true;
}

bool Database::connectFromConfig(const QString &configPath){
    if (!QFile::exists(configPath)){
        m_lastError = QString("Configuration file not found: %1").arg(configPath);
        qDebug() << m_lastError;
        return false;
    }

    QSettings settings(configPath, QSettings::IniFormat);

    QString host = settings.value("database/host", "localhost").toString();
    int port = settings.value("database/port", 5432).toInt();
    QString dbName = settings.value("database/name", "train_tickets").toString();
    QString username = settings.value("database/username", "").toString();
    QString password = settings.value("database/password", "").toString();

    if (username.isEmpty()){
        m_lastError = "Username does not exist";
        qDebug() << m_lastError;
        return false;
    }

    return connect(host, port, dbName, username, password);
}

void Database::disconnect(){
    QMutexLocker locker(&m_mutex);

    if (m_db.isOpen()){
        m_db.close();
        qDebug() << "Disconnecting from Database...";
    }
}

bool Database::isConnectedInternal() const {
    return m_db.isOpen();
}

bool Database::isConnected() const{
    QMutexLocker locker(&m_mutex);
    return isConnectedInternal();
}

bool Database::initializeTables(){
    QSqlQuery query(m_db);

    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            surname VARCHAR(100) NOT NULL,
            email VARCHAR(255) UNIQUE NOT NULL,
            password_hash VARCHAR(255) NOT NULL,
            password_salt VARCHAR(32) NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            is_verified BOOLEAN DEFAULT FALSE,
            last_login TIMESTAMP,
            failed_login_attempts INTEGER DEFAULT 0,
            locked_until TIMESTAMP,
            CONSTRAINT email_format CHECK (email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}$')
        )
    )";

    if (!query.exec(createUsersTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'users':" << m_lastError;
        return false;
    }

    QString createAuditTable = R"(
        CREATE TABLE IF NOT EXISTS audit_logs (
            id SERIAL PRIMARY KEY,
            user_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
            action VARCHAR(100) NOT NULL,
            ip_address VARCHAR(45),
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            details TEXT,
            success BOOLEAN DEFAULT TRUE
        )
    )";

    if (!query.exec(createAuditTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'audit_logs':" << m_lastError;
        return false;
    }

    QString createStationsTable = R"(
        CREATE TABLE IF NOT EXISTS stations (
            id SERIAL PRIMARY KEY,
            name VARCHAR(255) NOT NULL,
            city VARCHAR(100) NOT NULL,
            code VARCHAR(10) UNIQUE NOT NULL,
            latitude DOUBLE PRECISION NOT NULL,
            longitude DOUBLE PRECISION NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createStationsTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'stations':" << m_lastError;
        return false;
    }

    QString createSessionsTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id SERIAL PRIMARY KEY,
            user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            session_token VARCHAR(64) UNIQUE NOT NULL,
            ip_address VARCHAR(45),
            user_agent TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            expires_at TIMESTAMP NOT NULL,
            is_active BOOLEAN DEFAULT TRUE
        )
    )";

    if (!query.exec(createSessionsTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'sessions':" << m_lastError;
        return false;
    }

    QString createVerificationCodesTable = R"(
        CREATE TABLE IF NOT EXISTS verification_codes (
            id SERIAL PRIMARY KEY,
            user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            email VARCHAR(255) NOT NULL,
            code VARCHAR(6) NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            expires_at TIMESTAMP NOT NULL,
            is_used BOOLEAN DEFAULT FALSE
        )
    )";

    if (!query.exec(createVerificationCodesTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'verification_codes':" << m_lastError;
        return false;
    }

    QString createTrainsTable = R"(
        CREATE TABLE IF NOT EXISTS trains (
            id SERIAL PRIMARY KEY,
            train_number VARCHAR(20) UNIQUE NOT NULL,
            train_type VARCHAR(50) NOT NULL,
            total_seats INTEGER NOT NULL,
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (!query.exec(createTrainsTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'trains':" << m_lastError;
        return false;
    }

    QString createCarriagesTable = R"(
        CREATE TABLE IF NOT EXISTS carriages (
            id SERIAL PRIMARY KEY,
            train_id INTEGER NOT NULL REFERENCES trains(id) ON DELETE CASCADE,
            carriage_number INTEGER NOT NULL,
            carriage_type VARCHAR(50) NOT NULL,
            total_seats INTEGER NOT NULL,
            price_multiplier DOUBLE PRECISION DEFAULT 1.0,
            UNIQUE(train_id, carriage_number)
        )
    )";
    if (!query.exec(createCarriagesTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'carriages':" << m_lastError;
        return false;
    }

    QString createSeatsTable = R"(
        CREATE TABLE IF NOT EXISTS seats (
            id SERIAL PRIMARY KEY,
            carriage_id INTEGER NOT NULL REFERENCES carriages(id) ON DELETE CASCADE,
            seat_number INTEGER NOT NULL,
            seat_type VARCHAR(50) NOT NULL,
            is_available BOOLEAN DEFAULT TRUE,
            UNIQUE(carriage_id, seat_number)
        )
    )";
    if (!query.exec(createSeatsTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'seats':" << m_lastError;
        return false;
    }

    QString createRoutesTable = R"(
        CREATE TABLE IF NOT EXISTS routes (
            id SERIAL PRIMARY KEY,
            train_id INTEGER NOT NULL REFERENCES trains(id) ON DELETE CASCADE,
            route_name VARCHAR(255) NOT NULL,
            valid_from DATE NOT NULL,
            valid_to DATE NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (!query.exec(createRoutesTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'routes':" << m_lastError;
        return false;
    }

    QString createRouteStopsTable = R"(
        CREATE TABLE IF NOT EXISTS route_stops (
            id SERIAL PRIMARY KEY,
            route_id INTEGER NOT NULL REFERENCES routes(id) ON DELETE CASCADE,
            station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE CASCADE,
            stop_order INTEGER NOT NULL,
            arrival_time TIME,
            departure_time TIME NOT NULL,
            stop_duration_minutes INTEGER DEFAULT 0,
            price_from_start DOUBLE PRECISION NOT NULL,
            UNIQUE(route_id, stop_order),
            UNIQUE(route_id, station_id)
        )
    )";
    if (!query.exec(createRouteStopsTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'route_stops':" << m_lastError;
        return false;
    }

        QString createSchedulesTable = R"(
        CREATE TABLE IF NOT EXISTS schedules (
            id SERIAL PRIMARY KEY,
            route_id INTEGER NOT NULL REFERENCES routes(id) ON DELETE CASCADE,
            departure_date DATE NOT NULL,
            status VARCHAR(20) DEFAULT 'active',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(route_id, departure_date)
        )
    )";
    if (!query.exec(createSchedulesTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'schedules':" << m_lastError;
        return false;
    }

    QString createTicketsTable = R"(
        CREATE TABLE IF NOT EXISTS tickets (
            id SERIAL PRIMARY KEY,
            user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            schedule_id INTEGER NOT NULL REFERENCES schedules(id) ON DELETE CASCADE,
            seat_id INTEGER NOT NULL REFERENCES seats(id) ON DELETE CASCADE,
            departure_station_id INTEGER NOT NULL REFERENCES stations(id),
            arrival_station_id INTEGER NOT NULL REFERENCES stations(id),
            ticket_number VARCHAR(20) UNIQUE NOT NULL,
            price DOUBLE PRECISION NOT NULL,
            status VARCHAR(20) DEFAULT 'booked',
            booked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            paid_at TIMESTAMP,
            cancelled_at TIMESTAMP,
            passenger_name VARCHAR(200) NOT NULL,
            passenger_document VARCHAR(50) NOT NULL
        )
    )";
    if (!query.exec(createTicketsTable)){
        m_lastError = query.lastError().text();
        qDebug() << "Error during creating table 'tickets':" << m_lastError;
        return false;
    }

    query.exec("CREATE INDEX IF NOT EXISTS idx_users_email ON users(email)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_users_locked_until ON users(locked_until)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_logs(user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_audit_timestamp ON audit_logs(timestamp)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_sessions_expires ON sessions(expires_at)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_stations_code ON stations(code)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_trains_number ON trains(train_number)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_routes_train ON routes(train_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_route_stops_route ON route_stops(route_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_schedules_route_date ON schedules(route_id, departure_date)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_tickets_user ON tickets(user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_tickets_schedule ON tickets(schedule_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_tickets_number ON tickets(ticket_number)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_tickets_status ON tickets(status)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_verification_codes_user ON verification_codes(user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_verification_codes_code ON verification_codes(code)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_verification_codes_expires ON verification_codes(expires_at)");
    qDebug() << "All tables initialized successfully";
    return true;
}

QString Database::generateSalt(){
    QByteArray salt;
    for (int i = 0; i < PASSWORD_SALT_LENGTH; i++){
        salt.append(QRandomGenerator::global()->bounded(256));
    }
    return salt.toHex();
}

QString Database::hashPassword(const QString &password, const QString &salt){
    QString saltUse = salt.isEmpty() ? generateSalt() : salt;
    QString combined = password + saltUse;
    QByteArray hash = combined.toUtf8();
    for (int i = 0; i < 10000; i++){
        hash = QCryptographicHash::hash(hash, QCryptographicHash::Sha256);
    }
    return hash.toHex();
}

QString Database::generateToken(int length){
    return QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(length);
}

User Database::getUserByEmailInternal(const QString& email, bool* found) {
    User user;
    user.id = -1;

    if (! m_db.isOpen()) {
        if (found) *found = false;
        return user;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, name, surname, email, password_hash, password_salt,
               created_at, is_verified, last_login,
               failed_login_attempts, locked_until
        FROM users
        WHERE email = :email
    )");

    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Error getting user:" << m_lastError;
        if (found) *found = false;
        return user;
    }

    if (query.next()) {
        user.id = query.value("id").toInt();
        user.name = query.value("name").toString();
        user.surname = query.value("surname").toString();
        user.email = query.value("email").toString();
        user.passwordHash = query. value("password_hash").toString();
        user.passwordSalt = query.value("password_salt").toString();
        user.createdAt = query.value("created_at").toDateTime();
        user.isVerified = query. value("is_verified").toBool();
        user.lastLogin = query.value("last_login").toDateTime();
        user.failedLoginAttempts = query. value("failed_login_attempts").toInt();
        user.lockedUntil = query.value("locked_until").toDateTime();

        if (found) *found = true;
    } else {
        if (found) *found = false;
    }

    return user;
}

User Database::getUserByIdInternal(int id, bool* found) {
    User user;
    user.id = -1;

    if (!m_db.isOpen()) {
        if (found) *found = false;
        return user;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, name, surname, email, password_hash, password_salt,
               created_at, is_verified, last_login,
               failed_login_attempts, locked_until
        FROM users
        WHERE id = :id
    )");

    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        user.id = query.value("id").toInt();
        user.name = query. value("name").toString();
        user.surname = query.value("surname").toString();
        user.email = query.value("email").toString();
        user.passwordHash = query.value("password_hash").toString();
        user.passwordSalt = query.value("password_salt").toString();
        user.createdAt = query.value("created_at").toDateTime();
        user.isVerified = query.value("is_verified").toBool();
        user.lastLogin = query.value("last_login").toDateTime();
        user.failedLoginAttempts = query.value("failed_login_attempts").toInt();
        user.lockedUntil = query.value("locked_until").toDateTime();

        if (found) *found = true;
    } else {
        if (found) *found = false;
    }

    return user;
}

bool Database::userExistsInternal(const QString& email) {
    if (!m_db.isOpen()) return false;

    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM users WHERE email = :email");
    query.bindValue(":email", email. toLower().trimmed());

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

bool Database::isAccountLockedInternal(const QString& email) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT locked_until, failed_login_attempts
        FROM users
        WHERE email = :email
    )");

    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec() || ! query.next()) {
        return false;
    }

    QDateTime lockedUntil = query.value("locked_until").toDateTime();
    int attempts = query.value("failed_login_attempts").toInt();

    if (lockedUntil.isValid() && lockedUntil > QDateTime::currentDateTime()) {
        qDebug() << "Account is locked:" << email << "until" << lockedUntil.toString();
        return true;
    }

    if (attempts >= MAX_FAILED_ATTEMPTS) {
        lockAccountInternal(email, LOCKOUT_DURATION_MINUTES);
        return true;
    }

    return false;
}

void Database::incrementFailedAttemptsInternal(const QString& email) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE users
        SET failed_login_attempts = failed_login_attempts + 1
        WHERE email = :email
    )");

    query.bindValue(":email", email.toLower().trimmed());

    if (! query.exec()) {
        qDebug() << "Error updating attempts counter:" << query.lastError().text();
    } else {
        qDebug() << "Attempts counter was incremented for:" << email;
    }
}

void Database::resetFailedAttemptsInternal(const QString& email) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE users
        SET failed_login_attempts = 0, locked_until = NULL
        WHERE email = :email
    )");

    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec()) {
        qWarning() << "Error resetting attempts counter:" << query.lastError().text();
    }
}

void Database::lockAccountInternal(const QString& email, int minutes) {
    QDateTime lockUntil = QDateTime::currentDateTime().addSecs(minutes * 60);

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE users
        SET locked_until = :locked_until
        WHERE email = :email
    )");

    query.bindValue(":locked_until", lockUntil);
    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec()) {
        qDebug() << "Error locking account:" << query.lastError().text();
    } else {
        qDebug() << "Account locked:" << email << "until" << lockUntil.toString();
        logActionInternal(-1, "account_locked", "", QString("Account locked until %1").arg(lockUntil.toString()), false);
    }
}

bool Database::setUserVerifiedInternal(const QString& email, bool verified) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET is_verified = :verified WHERE email = :email");
    query.bindValue(":verified", verified);
    query.bindValue(":email", email.toLower().trimmed());

    return query.exec();
}

bool Database::logActionInternal(int userId, const QString& action, const QString& ipAddress, const QString& details, bool success) {
    if (!m_db.isOpen()) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO audit_logs (user_id, action, ip_address, details, success)
        VALUES (:user_id, :action, :ip_address, :details, :success)
    )");

    query.bindValue(":user_id", userId > 0 ? userId : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":action", action);
    query.bindValue(":ip_address", ipAddress. isEmpty() ? QVariant(QMetaType::fromType<QString>()) : ipAddress);
    query.bindValue(":details", details);
    query.bindValue(":success", success);

    if (!query.exec()) {
        qWarning() << "Error logging action:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::isSeatOccupiedInternal(int seatId, int scheduleId, int departureStationId, int arrivalStationId) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT COUNT(*) FROM tickets
        WHERE seat_id = :seat_id
          AND schedule_id = :schedule_id
          AND status IN ('booked', 'paid')
          AND (
              (departure_station_id <= :dep_station AND arrival_station_id > :dep_station)
              OR (departure_station_id < :arr_station AND arrival_station_id >= :arr_station)
              OR (departure_station_id >= :dep_station AND arrival_station_id <= :arr_station)
          )
    )");

    query.bindValue(":seat_id", seatId);
    query.bindValue(":schedule_id", scheduleId);
    query.bindValue(":dep_station", departureStationId);
    query.bindValue(":arr_station", arrivalStationId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

User Database::getUserByEmail(const QString& email, bool* found) {
    QMutexLocker locker(&m_mutex);
    return getUserByEmailInternal(email, found);
}

User Database:: getUserById(int id, bool* found) {
    QMutexLocker locker(&m_mutex);
    return getUserByIdInternal(id, found);
}

bool Database::userExists(const QString& email) {
    QMutexLocker locker(&m_mutex);
    return userExistsInternal(email);
}

bool Database:: isAccountLocked(const QString& email) {
    QMutexLocker locker(&m_mutex);
    return isAccountLockedInternal(email);
}

void Database::incrementFailedAttempts(const QString& email) {
    QMutexLocker locker(&m_mutex);
    incrementFailedAttemptsInternal(email);
}

void Database::resetFailedAttempts(const QString& email) {
    QMutexLocker locker(&m_mutex);
    resetFailedAttemptsInternal(email);
}

void Database::lockAccount(const QString& email, int minutes) {
    QMutexLocker locker(&m_mutex);
    lockAccountInternal(email, minutes);
    emit accountLocked(email);
    emit securityAlert(QString("Account locked due to failed attempts:  %1").arg(email));
}

bool Database::setUserVerified(const QString& email, bool verified) {
    QMutexLocker locker(&m_mutex);
    return setUserVerifiedInternal(email, verified);
}

bool Database::logAction(int userId, const QString& action, const QString& ipAddress,
                         const QString& details, bool success) {
    QMutexLocker locker(&m_mutex);
    return logActionInternal(userId, action, ipAddress, details, success);
}

bool Database::createUser(const QString& name, const QString& surname, const QString& email, const QString& password, int* userId){
    QMutexLocker locker(&m_mutex);

    if (!isConnectedInternal()){
        m_lastError = "No connection to database";
        return false;
    }

    if (!isValidEmail(email)){
        m_lastError = "Invalid email format";
        return false;
    }

    if(password.length() < 8){
        m_lastError = "Password length must be longer equal, than 8";
        return false;
    }

    if (userExistsInternal(email)){
        m_lastError = "User with that email is already exusts";
        logActionInternal(-1, "register_failed", "", QString("Email already exists: %1").arg(email), false);
        return false;
    }

    QString salt = generateSalt();
    QString passHash = hashPassword(password, salt);

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO users (name, surname, email, password_hash, password_salt, is_verified)
        VALUES (:name, :surname, :email, :password_hash, :salt, :is_verified)
        RETURNING id
    )");

    query.bindValue(":name", sanitizeInput(name));
    query.bindValue(":surname", sanitizeInput(surname));
    query.bindValue(":email", email.toLower().trimmed());
    query.bindValue(":password_hash", passHash);
    query.bindValue(":salt", salt);
    query.bindValue(":is_verified", false);

    if (!query.exec()){
        m_lastError = query.lastError().text();
        qDebug() << "Error while creating user: " << m_lastError;
        logActionInternal(-1, "register_failed", "", m_lastError, false);
        return false;
    }

    int newUserId = -1;
    if (query.next()){
        newUserId = query.value(0).toInt();
    }

    if (userId != nullptr){
        *userId = newUserId;
    }

    qDebug() << "User was created" << email << "ID: " << newUserId;

    logActionInternal(newUserId, "register", "", "User registered successfully", true);
    emit userCreated(newUserId);
    return true;
}

bool Database::checkPassword(const QString &email, const QString &password, QString *errorMsg){
    QMutexLocker locker(&m_mutex);

    if (!isConnectedInternal()){
        if (errorMsg) *errorMsg = "No connection to database";
        return false;
    }

    if (isAccountLockedInternal(email)){
        if (errorMsg) *errorMsg = "Account is temporarily locked due to multiple failed login settings";
        logActionInternal(-1, "login_blocked", "", email, false);
        emit securityAlert(QString("Login attempt to locked account: %1").arg(email));
        return false;
    }

    bool found;
    User user = getUserByEmailInternal(email, &found);

    if (!found){
        if (errorMsg) *errorMsg = "Invalid email or password";
        logActionInternal(-1, "login_failed", "", QString("User not found: %1").arg(email), false);
        return false;
    }

    QString inputHash = hashPassword(password, user.passwordSalt);

    if (user.passwordHash != inputHash){
        incrementFailedAttemptsInternal(email);
        if (errorMsg) *errorMsg = "Invalid email or password";
        logActionInternal(user.id, "login_failed", "", "Incorrect password", false);
        return false;
    }

    resetFailedAttemptsInternal(email);
    qDebug() << "Authorization successed: " << email;
    return true;
}

bool Database::updateLastLogin(const QString &email, const QString& ipAddress){
    QMutexLocker locker(&m_mutex);

    if (!isConnectedInternal()) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE users
        SET last_login = CURRENT_TIMESTAMP
        WHERE email = :email
    )");

    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    bool found;
    User user = getUserByEmailInternal(email, &found);
    if (found){
        logActionInternal(user.id, "login", ipAddress, "Successful login", true);
    }
    return true;
}

bool Database::changePassword(const QString& email, const QString& oldPassword, const QString& newPassword){
    QMutexLocker locker(&m_mutex);

    if (!m_db.isOpen()){
        m_lastError = "No connection to database";
        return false;
    }

    bool found;
    User user = getUserByEmailInternal(email, &found);

    if (!found) {
        m_lastError = "User not found";
        return false;
    }

    QString oldHash = hashPassword(oldPassword, user.passwordSalt);
    if (user.passwordHash != oldHash){
        m_lastError = "Wrong current password";
        return false;
    }

    if (newPassword.length() < 8){
        m_lastError = "New password must be at least 8 characters";
        return false;
    }

    QString newSalt = generateSalt();
    QString newHash = hashPassword(newPassword, newSalt);

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE users
        SET password_hash = :hash, password_salt = :salt
        WHERE email = :email
    )");

    query.bindValue(":hash", newHash);
    query.bindValue(":salt", newSalt);
    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    logActionInternal(user.id, "password_changed", "", "Password changed successfully", true);
    qDebug() << "Password was changed for" << email;
    return true;
}

bool Database::resetPassword(const QString &email, const QString &newPassword){
    QMutexLocker locker(&m_mutex);

    if (newPassword.length() < 8){
        m_lastError = "Password must be equal longer than 8";
        return false;
    }

    QString newSalt = generateSalt();
    QString newHash = hashPassword(newPassword, newSalt);

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE users
        SET password_hash = :hash,
            password_salt = :salt,
            failed_login_attempts = 0,
            locked_until = NULL
        WHERE email = :email
    )");

    query.bindValue(":hash", newHash);
    query.bindValue(":salt", newSalt);
    query.bindValue(":email", email.toLower().trimmed());

    if (!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    bool found;
    User user = getUserByEmailInternal(email, &found);
    if (found){
        logActionInternal(user.id, "password_reset", "", "Password reset", true);
    }
    qDebug() << "Password was changed for " << email;
    return true;
}

QList<AuditLog> Database::getAuditLogs(int userId, int limit){
    QMutexLocker locker(&m_mutex);
    QList<AuditLog> logs;
    if (!isConnectedInternal()) return logs;

    QSqlQuery query(m_db);
    if (userId > 0){
        query.prepare(R"(
            SELECT id, user_id, action, ip_address, timestamp, details, success
            FROM audit_logs
            WHERE user_id = :user_id
            ORDER BY timestamp DESC
            LIMIT :limit
        )");
        query.bindValue(":user_id", userId);
    } else {
        query.prepare(R"(
            SELECT id, user_id, action, ip_address, timestamp, details, success
            FROM audit_logs
            ORDER BY timestamp DESC
            LIMIT :limit
        )");
    }
    query.bindValue(":limit", limit);
    if (!query.exec()) {
        qDebug() << "Error getting logs: " << query.lastError().text();
        return logs;
    }

    while (query.next()){
        AuditLog log;
        log.id = query.value("id").toInt();
        log.userId = query.value("user_id").toInt();
        log.action = query.value("action").toString();
        log.ipAddress = query.value("ip_address").toString();
        log.timestamp = query.value("timestamp").toDateTime();
        log.details = query.value("details").toString();
        log.success = query.value("success").toBool();
        logs.append(log);
    }
    return logs;
}

QString Database::createSession(int userId, const QString &ipAddress, const QString &userAgent){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return QString();

    QString sessionToken = generateToken(32);
    QDateTime expiresAt = QDateTime::currentDateTime().addSecs(SESSION_LIFETIME_HOURS * 3600);

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO sessions (user_id, session_token, ip_address, user_agent, expires_at)
        VALUES (:user_id, :token, :ip, :ua, :expires)
    )");

    query.bindValue(":user_id", userId);
    query.bindValue(":token", sessionToken);
    query.bindValue(":ip", ipAddress);
    query.bindValue(":ua", userAgent);
    query.bindValue(":expires", expiresAt);

    if (!query.exec()){
        qDebug() << "Error creating session: " << query.lastError().text();
        return QString();
    }

    qDebug() << "Session created for user: " << userId;
    return sessionToken;
}

bool Database::validateSession(const QString &sessionToken, int *userId){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT user_id, expires_at, is_active
        FROM sessions
        WHERE session_token = :token
    )");

    query.bindValue(":token", sessionToken);

    if (!query.exec() || !query.next()) {
        return false;
    }

    bool isActive = query.value("is_active").toBool();
    QDateTime expiresAt = query.value("expires_at").toDateTime();
    if (!isActive || expiresAt < QDateTime::currentDateTime()) {
        return false;
    }
    if (userId != nullptr) {
        *userId = query.value("user_id").toInt();
    }
    return true;
}

bool Database::invalidateSession(const QString &sessionToken){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return false;

    QSqlQuery query(m_db);
    query.prepare("UPDATE sessions SET is_active = FALSE WHERE session_token = :token");
    query.bindValue(":token", sessionToken);
    return query.exec();
}

void Database::cleanupExpiredSessions(){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP");
    if (query.exec()) {
        int deleted = query.numRowsAffected();
        if (deleted > 0) {
            qDebug() << "Expired sessions deleted: " << deleted;
        }
    }
}

QString Database::createVerificationCode(int userId, const QString& email){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return QString();

    QString code = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
    QDateTime expiresAt = QDateTime::currentDateTime().addSecs(15 * 60);

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO verification_codes (user_id, email, code, expires_at)
        VALUES (:user_id, :email, :code, :expires_at)
    )");

    query.bindValue(":user_id", userId);
    query.bindValue(":email", email.toLower().trimmed());
    query.bindValue(":code", code);
    query.bindValue(":expires_at", expiresAt);

    if (!query.exec()){
        m_lastError = query.lastError().text();
        qDebug() << "Error creating verification code:" << m_lastError;
        return QString();
    }

    qDebug() << "Verification code created for user:" << userId << "Code:" << code;
    return code;
}

bool Database::verifyEmail(const QString& email, const QString& code, QString* errorMsg){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()){
        if (errorMsg) *errorMsg = "No connection to database";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT user_id, expires_at, is_used
        FROM verification_codes
        WHERE email = :email AND code = :code
        ORDER BY created_at DESC
        LIMIT 1
    )");

    query.bindValue(":email", email.toLower().trimmed());
    query.bindValue(":code", code);

    if (!query.exec() || !query.next()){
        if (errorMsg) *errorMsg = "Invalid verification code";
        logActionInternal(-1, "verification_failed", "", QString("Invalid code for %1").arg(email), false);
        return false;
    }

    bool isUsed = query.value("is_used").toBool();
    QDateTime expiresAt = query.value("expires_at").toDateTime();
    int userId = query.value("user_id").toInt();

    if (isUsed){
        if (errorMsg) *errorMsg = "Verification code already used";
        return false;
    }

    if (expiresAt < QDateTime::currentDateTime()){
        if (errorMsg) *errorMsg = "Verification code expired";
        return false;
    }

    QSqlQuery updateQuery(m_db);
    updateQuery.prepare(R"(
        UPDATE verification_codes
        SET is_used = TRUE
        WHERE email = :email AND code = :code
    )");
    updateQuery.bindValue(":email", email.toLower().trimmed());
    updateQuery.bindValue(":code", code);
    updateQuery.exec();

    setUserVerifiedInternal(email, true);

    logActionInternal(userId, "email_verified", "", "Email verified successfully", true);
    qDebug() << "Email verified for user:" << userId;

    return true;
}

void Database::cleanupExpiredVerificationCodes(){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM verification_codes WHERE expires_at < CURRENT_TIMESTAMP");
    if (query.exec()){
        int deleted = query.numRowsAffected();
        if (deleted > 0){
            qDebug() << "Expired verification codes deleted:" << deleted;
        }
    }
}

QString Database::sanitizeInput(const QString &input){
    QString sanitized = input.trimmed();
    sanitized.remove(QRegularExpression("<[^>]*>"));
    sanitized.remove(QRegularExpression("[\\x00-\\x1F]"));
    return sanitized;
}

bool Database::isValidEmail(const QString& email)
{
    QRegularExpression regex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    return regex.match(email).hasMatch();
}

QString Database::lastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

int Database::createStation(const QString &name, const QString &city, const QString &code, double lat, double lon){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return -1;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO stations (name, city, code, latitude, longitude)
        VALUES (:name, :city, :code, :lat, :lon)
        RETURNING id
    )");

    query.bindValue(":name", sanitizeInput(name));
    query.bindValue(":city", sanitizeInput(city));
    query.bindValue(":code", code.toUpper().trimmed());
    query.bindValue(":lat", lat);
    query.bindValue(":lon", lon);

    if (!query.exec() || !query.next()){
        m_lastError = query.lastError().text();
        qDebug() << "Error creating station:" << m_lastError;
        return -1;
    }

    int stationId = query.value(0).toInt();
    qDebug() << "Station created:" << name << "ID:" << stationId;
    return stationId;
}

Station Database::getStation(int stationId, bool *found){
    QMutexLocker locker(&m_mutex);
    Station station;
    station.id = -1;

    if (!isConnectedInternal()){
        if (found) *found = false;
        return station;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM stations WHERE id = :id");
    query.bindValue(":id", stationId);

    if (!query.exec() || !query.next()){
        if (found) *found = false;
        return station;
    }

    station.id = query.value("id").toInt();
    station.name = query.value("name").toString();
    station.city = query.value("city").toString();
    station.code = query.value("code").toString();
    station.latitude = query.value("latitude").toDouble();
    station.longitude = query.value("longitude").toDouble();

    if (found) *found = true;
    return station;
}

QList<Station> Database::getAllStations(){
    QMutexLocker locker(&m_mutex);
    QList<Station> stations;
    if (!isConnectedInternal()) return stations;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM stations ORDER BY name");

    if (!query.exec()){
        qDebug() << "Error getting stations:" << query.lastError().text();
        return stations;
    }

    while (query.next()){
        Station station;
        station.id = query.value("id").toInt();
        station.name = query.value("name").toString();
        station.city = query.value("city").toString();
        station.code = query.value("code").toString();
        station.latitude = query.value("latitude").toDouble();
        station.longitude = query.value("longitude").toDouble();
        stations.append(station);
    }
    return stations;
}

QList<Station> Database::searchStations(const QString &searchText){
    QMutexLocker locker(&m_mutex);
    QList<Station> stations;
    if (!isConnectedInternal()) return stations;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT * FROM stations
        WHERE LOWER(name) LIKE :search
           OR LOWER(city) LIKE :search
           OR LOWER(code) LIKE :search
        ORDER BY name
    )");

    QString search = "%" + searchText.toLower() + "%";
    query.bindValue(":search", search);

    if (!query.exec()){
        qDebug() << "Error searching stations:" << query.lastError().text();
        return stations;
    }

    while (query.next()){
        Station station;
        station.id = query.value("id").toInt();
        station.name = query.value("name").toString();
        station.city = query.value("city").toString();
        station.code = query.value("code").toString();
        station.latitude = query.value("latitude").toDouble();
        station.longitude = query.value("longitude").toDouble();
        stations.append(station);
    }
    return stations;
}

int Database::createTrain(const QString &trainNumber, const QString &trainType, int totalSeats){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return -1;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO trains (train_number, train_type, total_seats)
        VALUES (:number, :type, :seats)
        RETURNING id
    )");

    query.bindValue(":number", trainNumber.trimmed());
    query.bindValue(":type", trainType);
    query.bindValue(":seats", totalSeats);

    if (!query.exec() || !query.next()){
        m_lastError = query.lastError().text();
        qDebug() << "Error creating train:" << m_lastError;
        return -1;
    }

    int trainId = query.value(0).toInt();
    qDebug() << "Train created:" << trainNumber << "ID:" << trainId;
    return trainId;
}

Train Database::getTrain(int trainId, bool *found){
    QMutexLocker locker(&m_mutex);
    Train train;
    train.id = -1;

    if (!isConnectedInternal()){
        if (found) *found = false;
        return train;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM trains WHERE id = :id");
    query.bindValue(":id", trainId);

    if (!query.exec() || !query.next()){
        if (found) *found = false;
        return train;
    }

    train.id = query.value("id").toInt();
    train.trainNumber = query.value("train_number").toString();
    train.trainType = query.value("train_type").toString();
    train.totalSeats = query.value("total_seats").toInt();
    train.isActive = query.value("is_active").toBool();

    if (found) *found = true;
    return train;
}

QList<Train> Database::getAllTrains(){
    QMutexLocker locker(&m_mutex);
    QList<Train> trains;
    if (!isConnectedInternal()) return trains;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM trains ORDER BY train_number");

    if (!query.exec()){
        qDebug() << "Error getting trains:" << query.lastError().text();
        return trains;
    }

    while (query.next()){
        Train train;
        train.id = query.value("id").toInt();
        train.trainNumber = query.value("train_number").toString();
        train.trainType = query.value("train_type").toString();
        train.totalSeats = query.value("total_seats").toInt();
        train.isActive = query.value("is_active").toBool();
        trains.append(train);
    }
    return trains;
}

int Database::createRoute(int trainId, const QString &routeName, const QDate &validFrom, const QDate &validTo){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return -1;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO routes (train_id, route_name, valid_from, valid_to)
        VALUES (:train_id, :name, :from, :to)
        RETURNING id
    )");

    query.bindValue(":train_id", trainId);
    query.bindValue(":name", routeName);
    query.bindValue(":from", validFrom);
    query.bindValue(":to", validTo);

    if (!query.exec() || !query.next()){
        m_lastError = query.lastError().text();
        qDebug() << "Error creating route:" << m_lastError;
        return -1;
    }

    int routeId = query.value(0).toInt();
    qDebug() << "Route created:" << routeName << "ID:" << routeId;
    return routeId;
}

int Database::addRouteStop(int routeId, int stationId, int stopOrder, const QTime &arrival, const QTime &departure, int stopDuration, double priceFromStart){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return -1;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO route_stops
        (route_id, station_id, stop_order, arrival_time, departure_time,
         stop_duration_minutes, price_from_start)
        VALUES (:route_id, :station_id, :order, :arrival, :departure, :duration, :price)
        RETURNING id
    )");

    query.bindValue(":route_id", routeId);
    query.bindValue(":station_id", stationId);
    query.bindValue(":order", stopOrder);
    query.bindValue(":arrival", stopOrder == 1 ? QVariant(QVariant::Time) : arrival);
    query.bindValue(":departure", departure);
    query.bindValue(":duration", stopDuration);
    query.bindValue(":price", priceFromStart);

    if (!query.exec() || !query.next()){
        m_lastError = query.lastError().text();
        qDebug() << "Error adding route stop:" << m_lastError;
        return -1;
    }

    return query.value(0).toInt();
}

QList<RouteStop> Database::getRouteStops(int routeId){
    QMutexLocker locker(&m_mutex);
    QList<RouteStop> stops;
    if (!isConnectedInternal()) return stops;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT * FROM route_stops
        WHERE route_id = :route_id
        ORDER BY stop_order
    )");
    query.bindValue(":route_id", routeId);

    if (!query.exec()){
        qDebug() << "Error getting route stops:" << query.lastError().text();
        return stops;
    }

    while (query.next()){
        RouteStop stop;
        stop.id = query.value("id").toInt();
        stop.routeId = query.value("route_id").toInt();
        stop.stationId = query.value("station_id").toInt();
        stop.stopOrder = query.value("stop_order").toInt();
        stop.arrivalTime = query.value("arrival_time").toTime();
        stop.departureTime = query.value("departure_time").toTime();
        stop.stopDurationMinutes = query.value("stop_duration_minutes").toInt();
        stop.priceFromStart = query.value("price_from_start").toDouble();
        stops.append(stop);
    }
    return stops;
}

int Database::createSchedule(int routeId, const QDate &departureDate){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return -1;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO schedules (route_id, departure_date, status)
        VALUES (:route_id, :date, 'active')
        RETURNING id
    )");

    query.bindValue(":route_id", routeId);
    query.bindValue(":date", departureDate);

    if (!query.exec() || !query.next()){
        m_lastError = query.lastError().text();
        qDebug() << "Error creating schedule:" << m_lastError;
        return -1;
    }

    return query.value(0).toInt();
}

QList<Database::SearchResult> Database::searchTrains(int departureStationId, int arrivalStationId, const QDate &date){
    QMutexLocker locker(&m_mutex);
    QList<SearchResult> results;
    if (!isConnectedInternal()) return results;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT DISTINCT
            s.id as schedule_id,
            r.id as route_id,
            t.train_number,
            t.train_type,
            rs1.station_id as dep_station_id,
            st1.name as dep_station_name,
            rs2.station_id as arr_station_id,
            st2.name as arr_station_name,
            rs1.departure_time as dep_time,
            rs2.arrival_time as arr_time,
            (rs2.price_from_start - rs1.price_from_start) as min_price
        FROM schedules s
        JOIN routes r ON s.route_id = r.id
        JOIN trains t ON r.train_id = t.id
        JOIN route_stops rs1 ON r.id = rs1.route_id AND rs1.station_id = :dep_station
        JOIN route_stops rs2 ON r.id = rs2.route_id AND rs2.station_id = :arr_station
        JOIN stations st1 ON rs1.station_id = st1.id
        JOIN stations st2 ON rs2.station_id = st2.id
        WHERE s.departure_date = :date
          AND s.status = 'active'
          AND rs1.stop_order < rs2.stop_order
          AND t.is_active = true
          AND :date BETWEEN r.valid_from AND r.valid_to
        ORDER BY rs1.departure_time
    )");

    query.bindValue(":dep_station", departureStationId);
    query.bindValue(":arr_station", arrivalStationId);
    query.bindValue(":date", date);

    if (!query.exec()){
        m_lastError = query.lastError().text();
        qDebug() << "Error searching trains:" << m_lastError;
        return results;
    }

    while (query.next()){
        SearchResult result;
        result.scheduleId = query.value("schedule_id").toInt();
        result.routeId = query.value("route_id").toInt();
        result.trainNumber = query.value("train_number").toString();
        result.trainType = query.value("train_type").toString();
        result.departureStationId = query.value("dep_station_id").toInt();
        result.departureStationName = query.value("dep_station_name").toString();
        result.arrivalStationId = query.value("arr_station_id").toInt();
        result.arrivalStationName = query.value("arr_station_name").toString();

        QTime depTime = query.value("dep_time").toTime();
        QTime arrTime = query.value("arr_time").toTime();
        result.departureTime = QDateTime(date, depTime);
        result.arrivalTime = QDateTime(date, arrTime);

        if (arrTime < depTime){
            result.arrivalTime = result.arrivalTime.addDays(1);
        }

        result.travelTimeMinutes = result.departureTime.secsTo(result.arrivalTime) / 60;
        result.minPrice = query.value("min_price").toDouble();

        QSqlQuery seatQuery(m_db);
        seatQuery.prepare(R"(
            SELECT COUNT(DISTINCT s.id)
            FROM seats s
            JOIN carriages c ON s.carriage_id = c.id
            JOIN trains t ON c.train_id = t.id
            JOIN routes r ON t.id = r.train_id
            WHERE r.id = :route_id
              AND s.id NOT IN (
                  SELECT seat_id FROM tickets
                  WHERE schedule_id = :schedule_id
                    AND status IN ('booked', 'paid')
              )
        )");
        seatQuery.bindValue(":route_id", result.routeId);
        seatQuery.bindValue(":schedule_id", result.scheduleId);

        if (seatQuery.exec() && seatQuery.next()){
            result.availableSeats = seatQuery.value(0).toInt();
        } else {
            result.availableSeats = 0;
        }
        results.append(result);
    }
    return results;
}

QString Database::generateTicketNumber(){
    qint64 msec = QDateTime::currentMSecsSinceEpoch();
    QString random = QString::number(QRandomGenerator::global()->bounded(1000000), 10).rightJustified(6, '0');
    return QString("TK%1%2").arg(msec).arg(random);
}

QString Database::bookTicket(int userId, int scheduleId, int seatId, int departureStationId, int arrivalStationId, const QString &passengerName, const QString &passengerDocument, double price){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return QString();

    if (isSeatOccupiedInternal(seatId, scheduleId, departureStationId, arrivalStationId)){
        m_lastError = "Seat is already occupied";
        return QString();
    }

    QString ticketNumber = generateTicketNumber();

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO tickets
        (user_id, schedule_id, seat_id, departure_station_id, arrival_station_id,
         ticket_number, price, status, passenger_name, passenger_document)
        VALUES (:user_id, :schedule_id, :seat_id, :dep_station, :arr_station,
                :ticket_number, :price, 'booked', :passenger_name, :passenger_doc)
    )");

    query.bindValue(":user_id", userId);
    query.bindValue(":schedule_id", scheduleId);
    query.bindValue(":seat_id", seatId);
    query.bindValue(":dep_station", departureStationId);
    query.bindValue(":arr_station", arrivalStationId);
    query.bindValue(":ticket_number", ticketNumber);
    query.bindValue(":price", price);
    query.bindValue(":passenger_name", sanitizeInput(passengerName));
    query.bindValue(":passenger_doc", sanitizeInput(passengerDocument));

    if (!query.exec()){
        m_lastError = query.lastError().text();
        qDebug() << "Error booking ticket:" << m_lastError;
        return QString();
    }

    logActionInternal(userId, "ticket_booked", "", QString("Ticket %1 booked").arg(ticketNumber), true);
    emit ticketBooked(ticketNumber);
    qDebug() << "Ticket booked:" << ticketNumber;
    return ticketNumber;
}

Ticket Database::getTicket(const QString& ticketNumber, bool* found){
    QMutexLocker locker(&m_mutex);
    Ticket ticket;
    ticket.id = -1;

    if (!isConnectedInternal()){
        if (found) *found = false;
        return ticket;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM tickets WHERE ticket_number = :ticket_number");
    query.bindValue(":ticket_number", ticketNumber);

    if (!query.exec() || !query.next()){
        if (found) *found = false;
        return ticket;
    }

    ticket.id = query.value("id").toInt();
    ticket.userId = query.value("user_id").toInt();
    ticket.scheduleId = query.value("schedule_id").toInt();
    ticket.seatId = query.value("seat_id").toInt();
    ticket.departureStationId = query.value("departure_station_id").toInt();
    ticket.arrivalStationId = query.value("arrival_station_id").toInt();
    ticket.ticketNumber = query.value("ticket_number").toString();
    ticket.price = query.value("price").toDouble();
    ticket.status = query.value("status").toString();
    ticket.bookedAt = query.value("booked_at").toDateTime();
    ticket.paidAt = query.value("paid_at").toDateTime();
    ticket.cancelledAt = query.value("cancelled_at").toDateTime();
    ticket.passengerName = query.value("passenger_name").toString();
    ticket.passengerDocument = query.value("passenger_document").toString();

    if (found) *found = true;
    return ticket;
}

bool Database::payTicket(const QString &ticketNumber){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE tickets
        SET status = 'paid', paid_at = CURRENT_TIMESTAMP
        WHERE ticket_number = :ticket_number AND status = 'booked'
    )");
    query.bindValue(":ticket_number", ticketNumber);

    if (!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0){
        m_lastError = "Ticket not found or already paid";
        return false;
    }

    emit ticketPaid(ticketNumber);
    qDebug() << "Ticket paid:" << ticketNumber;
    return true;
}

bool Database::cancelTicket(const QString &ticketNumber, const QString &reason){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE tickets
        SET status = 'cancelled', cancelled_at = CURRENT_TIMESTAMP
        WHERE ticket_number = :ticket_number AND status IN ('booked', 'paid')
    )");
    query.bindValue(":ticket_number", ticketNumber);

    if (!query.exec()){
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0){
        m_lastError = "Ticket not found or already cancelled";
        return false;
    }

    emit ticketCancelled(ticketNumber);
    qDebug() << "Ticket cancelled:" << ticketNumber << "Reason:" << reason;
    return true;
}

QList<Ticket> Database::getUserTickets(int userId){
    QMutexLocker locker(&m_mutex);
    QList<Ticket> tickets;
    if (!isConnectedInternal()) return tickets;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT * FROM tickets
        WHERE user_id = :user_id
        ORDER BY booked_at DESC
    )");
    query.bindValue(":user_id", userId);

    if (!query.exec()){
        qDebug() << "Error getting user tickets:" << query.lastError().text();
        return tickets;
    }

    while (query.next()){
        Ticket ticket;
        ticket.id = query.value("id").toInt();
        ticket.userId = query.value("user_id").toInt();
        ticket.scheduleId = query.value("schedule_id").toInt();
        ticket.seatId = query.value("seat_id").toInt();
        ticket.departureStationId = query.value("departure_station_id").toInt();
        ticket.arrivalStationId = query.value("arrival_station_id").toInt();
        ticket.ticketNumber = query.value("ticket_number").toString();
        ticket.price = query.value("price").toDouble();
        ticket.status = query.value("status").toString();
        ticket.bookedAt = query.value("booked_at").toDateTime();
        ticket.paidAt = query.value("paid_at").toDateTime();
        ticket.cancelledAt = query.value("cancelled_at").toDateTime();
        ticket.passengerName = query.value("passenger_name").toString();
        ticket.passengerDocument = query.value("passenger_document").toString();
        tickets.append(ticket);
    }
    return tickets;
}

TicketFullInfo Database::getTicketFullInfo(const QString& ticketNumber, bool* found){
    QMutexLocker locker(&m_mutex);
    TicketFullInfo info;
    info.ticket.id = -1;

    if (!isConnected()) {
        if (found) *found = false;
        return info;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT
            tk.id, tk.user_id, tk.schedule_id, tk.seat_id,
            tk.departure_station_id, tk.arrival_station_id,
            tk.ticket_number, tk.price, tk.status,
            tk.booked_at, tk.paid_at, tk.cancelled_at,
            tk.passenger_name, tk.passenger_document,
            t.train_number, t.train_type,
            st_dep.name as dep_station_name,
            st_arr.name as arr_station_name,
            rs_dep.departure_time,
            rs_arr.arrival_time,
            s.departure_date,
            c.carriage_number,
            se.seat_number
        FROM tickets tk
        JOIN schedules s ON tk.schedule_id = s.id
        JOIN routes r ON s.route_id = r.id
        JOIN trains t ON r.train_id = t.id
        JOIN seats se ON tk.seat_id = se.id
        JOIN carriages c ON se.carriage_id = c.id
        JOIN route_stops rs_dep ON r.id = rs_dep.route_id
            AND rs_dep.station_id = tk.departure_station_id
        JOIN route_stops rs_arr ON r.id = rs_arr.route_id
            AND rs_arr.station_id = tk.arrival_station_id
        JOIN stations st_dep ON tk.departure_station_id = st_dep.id
        JOIN stations st_arr ON tk.arrival_station_id = st_arr.id
        WHERE tk.ticket_number = :ticket_number
    )");

    query.bindValue(":ticket_number", ticketNumber);

    if (!query.exec() || !query.next()) {
        if (found) *found = false;
        return info;
    }

    info.ticket.id = query.value("id").toInt();
    info.ticket.userId = query.value("user_id").toInt();
    info.ticket.scheduleId = query.value("schedule_id").toInt();
    info.ticket.seatId = query.value("seat_id").toInt();
    info.ticket.departureStationId = query.value("departure_station_id").toInt();
    info.ticket.arrivalStationId = query.value("arrival_station_id").toInt();
    info.ticket.ticketNumber = query.value("ticket_number").toString();
    info.ticket.price = query.value("price").toDouble();
    info.ticket.status = query.value("status").toString();
    info.ticket.bookedAt = query.value("booked_at").toDateTime();
    info.ticket.paidAt = query.value("paid_at").toDateTime();
    info.ticket.cancelledAt = query.value("cancelled_at").toDateTime();
    info.ticket.passengerName = query.value("passenger_name").toString();
    info.ticket.passengerDocument = query.value("passenger_document").toString();

    info.trainNumber = query.value("train_number").toString();
    info.trainType = query.value("train_type").toString();
    info.departureStationName = query.value("dep_station_name").toString();
    info.arrivalStationName = query.value("arr_station_name").toString();

    QTime depTime = query.value("departure_time").toTime();
    QTime arrTime = query.value("arrival_time").toTime();
    QDate scheduleDate = query.value("departure_date").toDate();

    info.departureTime = QDateTime(scheduleDate, depTime);
    info.arrivalTime = QDateTime(scheduleDate, arrTime);

    if (arrTime < depTime) {
        info.arrivalTime = info.arrivalTime.addDays(1);
    }

    info.carriageNumber = query.value("carriage_number").toInt();
    info.seatNumber = query.value("seat_number").toInt();

    if (found) *found = true;
    return info;
}

QList<Seat> Database::getAvailableSeats(int scheduleId, int departureStationId, int arrivalStationId){
    QMutexLocker locker(&m_mutex);
    QList<Seat> seats;
    if (!isConnectedInternal()) return seats;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT s.id, s.carriage_id, s.seat_number, s.seat_type,
               c.carriage_number, c.carriage_type
        FROM seats s
        JOIN carriages c ON s.carriage_id = c.id
        JOIN trains t ON c.train_id = t.id
        JOIN routes r ON t.id = r.train_id
        JOIN schedules sch ON r.id = sch.route_id
        WHERE sch.id = :schedule_id
        ORDER BY c.carriage_number, s.seat_number
    )");

    query.bindValue(":schedule_id", scheduleId);

    if (!query.exec()){
        m_lastError = query.lastError().text();
        qDebug() << "Error getting seats:" << m_lastError;
        return seats;
    }

    while (query.next()){
        Seat seat;
        seat.id = query.value("id").toInt();
        seat.carriageId = query.value("carriage_id").toInt();
        seat.carriageNumber = query.value("carriage_number").toInt();
        seat.carriageType = query.value("carriage_type").toString();
        seat.seatNumber = query.value("seat_number").toInt();
        seat.seatType = query.value("seat_type").toString();
        seat.isAvailable = !isSeatOccupiedInternal(seat.id, scheduleId, departureStationId, arrivalStationId);
        seats.append(seat);
    }

    return seats;
}

void Database::cleanupExpiredBookings(int timeoutMinutes){
    QMutexLocker locker(&m_mutex);
    if (!isConnectedInternal()) return;

    QSqlQuery query(m_db);
    QString sql = QString(R"(
        UPDATE tickets
        SET status = 'expired', cancelled_at = CURRENT_TIMESTAMP
        WHERE status = 'booked'
          AND booked_at < (CURRENT_TIMESTAMP - INTERVAL '%1 minutes')
    )").arg(timeoutMinutes);

    if (query.exec(sql)){
        int expired = query.numRowsAffected();
        if (expired > 0){
            qDebug() << "Expired bookings cleaned:" << expired;
        }
    }
}
