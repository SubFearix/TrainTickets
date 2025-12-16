#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QCryptographicHash>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <QMap>
#include <QRandomGenerator>

struct User {
    int id;
    QString name;
    QString surname;
    QString email;
    QString passwordHash;
    QString passwordSalt;
    QDateTime createdAt;
    bool isVerified;
    QDateTime lastLogin;
    int failedLoginAttempts;
    QDateTime lockedUntil;
};

struct AuditLog {
    int id;
    int userId;
    QString action;
    QString ipAddress;
    QDateTime timestamp;
    QString details;
    bool success;
};

struct Station {
    int id;
    QString name;
    QString city;
    QString code;
    double latitude;
    double longitude;
};

struct Train {
    int id;
    QString trainNumber;
    QString trainType;
    int totalSeats;
    bool isActive;
};

struct RouteStop {
    int id;
    int routeId;
    int stationId;
    int stopOrder;
    QTime arrivalTime;
    QTime departureTime;
    int stopDurationMinutes;
    double priceFromStart;
};

struct Route {
    int id;
    int trainId;
    QString routeName;
    QDate validFrom;
    QDate validTo;
    QList<RouteStop> stops;
};

struct Carriage {
    int id;
    int trainId;
    int carriageNumber;
    QString carriageType;
    int totalSeats;
    double priceMultiplier;
};

struct Seat {
    int id;
    int carriageId;
    int carriageNumber;
    QString carriageType;
    int seatNumber;
    QString seatType;
    bool isAvailable;
};

struct Ticket {
    int id;
    int userId;
    int scheduleId;
    int seatId;
    int departureStationId;
    int arrivalStationId;
    QString ticketNumber;
    double price;
    QString status;
    QDateTime bookedAt;
    QDateTime paidAt;
    QDateTime cancelledAt;
    QString passengerName;
    QString passengerDocument;
};

struct Schedule {
    int id;
    int routeId;
    QDate departureDate;
    QString status;
    QDateTime createdAt;
};

struct TicketFullInfo {
    Ticket ticket;
    QString trainNumber;
    QString trainType;
    QString departureStationName;
    QString arrivalStationName;
    QDateTime departureTime;
    QDateTime arrivalTime;
    int carriageNumber;
    int seatNumber;
};

class Database : public QObject{
    Q_OBJECT

public:
    static Database& instance();

    bool connect(const QString& host = "localhost"
                 , int port = 5432
                 , const QString& dbName = "train_tickets"
                 , const QString& username = "train_user"
                 , const QString& password = "");
    bool connectFromConfig(const QString& configPath = "config/database.conf");
    void disconnect();
    bool isConnected() const;

    bool createUser(const QString& name
                    , const QString& surname
                    , const QString& email
                    , const QString& password
                    , int* userId = nullptr);

    User getUserByEmail(const QString& email, bool* found = nullptr);
    User getUserById(int id, bool* found = nullptr);

    QString createVerificationCode(int userId, const QString& email);
    bool verifyEmail(const QString& email, const QString& code, QString* errorMsg = nullptr);
    void cleanupExpiredVerificationCodes();

    bool userExists(const QString& email);
    bool checkPassword(const QString& email, const QString& password, QString* errorMsg = nullptr);
    bool updateLastLogin(const QString& email, const QString& ipAddress);
    bool setUserVerified(const QString& email, bool verified = true);
    bool changePassword(const QString& email
                        , const QString& oldPassword
                        , const QString& newPassword);
    bool resetPassword(const QString& email, const QString& newPassword);

    bool isAccountLocked(const QString& email);
    void incrementFailedAttempts(const QString& email);
    void resetFailedAttempts(const QString& email);
    void lockAccount(const QString& email, int minutes = 5);

    bool logAction(int userId
                   , const QString& action
                   , const QString& ipAddress
                   , const QString& details = ""
                   , bool success = true);
    QList<AuditLog> getAuditLogs(int userId = -1, int limit = 100);

    QString createSession(int userId
                          , const QString& ipAddress
                          , const QString& userAgent = "");
    bool validateSession(const QString& sessionToken, int* userId = nullptr);
    bool invalidateSession(const QString& sessionToken);
    void cleanupExpiredSessions();

    int createStation(const QString& name
                      , const QString& city
                      , const QString& code
                      , double lat
                      , double lon);
    Station getStation(int stationId, bool* found = nullptr);
    QList<Station> getAllStations();
    QList<Station> searchStations(const QString& searchText);
    bool updateStation(int stationId
                       , const QString& name
                       , const QString& city
                       , const QString& code);

    int createTrain(const QString& trainNumber
                    , const QString& trainType
                    , int totalSeats);
    Train getTrain(int trainId, bool* found = nullptr);
    Train getTrainByNumber(const QString& trainNumber, bool* found = nullptr);
    QList<Train> getAllTrains();
    bool setTrainActive(int trainId, bool active);
    bool deleteTrain(int trainId);

    int createCarriage(int trainId
                       , int carriageNumber
                       , const QString& type
                       , int seats
                       , double priceMultiplier);
    QList<Carriage> getTrainCarriages(int trainId);
    bool deleteCarriage(int carriageId);

    int createSeat(int carriageId
                   , int seatNumber
                   , const QString& seatType);
    QList<Seat> getCarriageSeats(int carriageId);
    QList<Seat> getAvailableSeats(int scheduleId
                                  , int departureStationId
                                  , int arrivalStationId);
    bool setSeatAvailability(int seatId, bool available);

    int createRoute(int trainId
                    , const QString& routeName
                    , const QDate& validFrom
                    , const QDate& validTo);
    Route getRoute(int routeId, bool* found = nullptr);
    QList<Route> getTrainRoutes(int trainId);
    bool deleteRoute(int routeId);

    int addRouteStop(int routeId
                     , int stationId
                     , int stopOrder
                     , const QTime& arrival
                     , const QTime& departure
                     , int stopDuration
                     , double priceFromStart);
    QList<RouteStop> getRouteStops(int routeId);
    bool deleteRouteStop(int stopId);

    int createSchedule(int routeId, const QDate& departureDate);
    Schedule getSchedule(int scheduleId, bool* found = nullptr);
    QList<Schedule> getSchedulesByRoute(int routeId
                                        , const QDate& fromDate
                                        , const QDate& toDate);
    bool setScheduleStatus(int scheduleId, const QString& status);
    bool deleteSchedule(int scheduleId);

    struct SearchResult {
        int scheduleId;
        int routeId;
        QString trainNumber;
        QString trainType;
        int departureStationId;
        QString departureStationName;
        int arrivalStationId;
        QString arrivalStationName;
        QDateTime departureTime;
        QDateTime arrivalTime;
        int travelTimeMinutes;
        double minPrice;
        int availableSeats;
    };

    QList<SearchResult> searchTrains(int departureStationId
                                     , int arrivalStationId
                                     , const QDate& date);

    QString bookTicket(int userId
                       , int scheduleId
                       , int seatId
                       , int departureStationId
                       , int arrivalStationId
                       , const QString& passengerName
                       , const QString& passengerDocument
                       , double price);
    bool payTicket(const QString& ticketNumber);
    bool cancelTicket(const QString& ticketNumber, const QString& reason = "");
    Ticket getTicket(const QString& ticketNumber, bool* found = nullptr);
    Ticket getTicketById(int ticketId, bool* found = nullptr);
    QList<Ticket> getUserTickets(int userId);
    QList<Ticket> getScheduleTickets(int scheduleId);
    TicketFullInfo getTicketFullInfo(const QString& ticketNumber, bool* found = nullptr);

    void cleanupExpiredBookings(int timeoutMinutes = 15);

    static QString hashPassword(const QString& password, const QString& salt = "");
    static QString generateSalt();
    static QString generateToken(int length = 32);
    static QString generateTicketNumber();

    QString lastError() const;
    static QString sanitizeInput(const QString& input);
    static bool isValidEmail(const QString& email);

signals:
    void userCreated(int userId);
    void userDeleted(const QString& email);
    void connectionError(const QString& error);
    void securityAlert(const QString& message);
    void accountLocked(const QString& email);
    void ticketBooked(QString ticketNumber);
    void ticketCancelled(QString ticketNumber);
    void ticketPaid(QString ticketNumber);

private:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase m_db;
    QString m_lastError;
    mutable QRecursiveMutex m_mutex;

    static const int MAX_FAILED_ATTEMPTS = 5;
    static const int LOCKOUT_DURATION_MINUTES = 5;
    static const int SESSION_LIFETIME_HOURS = 24;
    static const int PASSWORD_SALT_LENGTH = 16;
    static const int BOOKING_TIMEOUT_MINUTES = 15;

    User getUserByEmailInternal(const QString& email, bool* found);
    User getUserByIdInternal(int id, bool* found);
    bool userExistsInternal(const QString& email);
    bool isConnectedInternal() const;
    bool isAccountLockedInternal(const QString& email);
    void incrementFailedAttemptsInternal(const QString& email);
    void resetFailedAttemptsInternal(const QString& email);
    void lockAccountInternal(const QString& email, int minutes);
    bool setUserVerifiedInternal(const QString& email, bool verified);
    bool logActionInternal(int userId
                           , const QString& action
                           , const QString& ipAddress
                           , const QString& details
                           , bool success);
    bool isSeatOccupiedInternal(int seatId
                                , int scheduleId
                                ,int departureStationId
                                , int arrivalStationId);
    bool initializeTables();
};

#endif
