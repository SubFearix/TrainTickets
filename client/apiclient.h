#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

struct Station{
    int id;
    QString name;
    QString city;
    QString code;
    double latitude;
    double longitude;
};

struct TrainSearchResult{
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

struct Seat{
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
    QString ticketNumber;
    int scheduleId;
    int seatId;
    int departureStationId;
    int arrivalStationId;
    double price;
    QString status;
    QString passengerName;
    QString passengerDocument;
    QDateTime bookedAt;
    QDateTime paidAt;
    QDateTime cancelledAt;
};

struct UserProfile {
    int id;
    QString name;
    QString surname;
    QString email;
    QDateTime createdAt;
    bool isVerified;
    QDateTime lastLogin;
};

class ApiClient : public QObject{
    Q_OBJECT

public:
    static ApiClient& instance() {
        static ApiClient inst;
        return inst;
    }

    void connectToServer(const QString& host = "127.0.0.1", quint16 port = 8080);
    void disconnectFromServer();
    bool isConnected() const;

    void registerUser(const QString& name
                      , const QString& surname
                      , const QString& email
                      , const QString& password);
    void login(const QString& email, const QString& password);
    void logout();
    void verifyEmail(const QString& email, const QString& code);
    void resendVerification(const QString& email);

    void getProfile();
    void changePassword(const QString& oldPassword, const QString& newPassword);

    void getStations(const QString& search = "");
    void searchTrains(int departureStationId, int arrivalStationId, const QDate& date);

    void getAvailableSeats(int scheduleId, int departureStationId, int arrivalStationId);
    void bookTicket(int scheduleId
                    , int seatId
                    , int departureStationId
                    , int arrivalStationId
                    , const QString& passengerName
                    , const QString& passengerDocument
                    , double price);
    void payTicket(const QString& ticketNumber);
    void cancelTicket(const QString& ticketNumber, const QString& reason = "");

    void getMyTickets();
    void getTicketDetails(const QString& ticketNumber);

    QString getSessionToken() const {return m_sessionToken;}
    UserProfile getUserProfile() const {return m_userProfile;}
    bool isAuthenticated() const {return m_authenticated;}

signals:
    void connected();
    void disconnected();
    void error(QString errorMessage);

    void registerSuccess(int userId, QString email, bool requiresVerification);
    void registerFailed(QString errorMessage);
    void loginSuccess(UserProfile user);
    void loginFailed(QString errorMessage);
    void logoutSuccess();
    void verificationSuccess();
    void verificationFailed(QString errorMessage);

    void stationsReceived(QList<Station> stations);
    void trainsReceived(QList<TrainSearchResult> trains);
    void seatsReceived(QList<Seat> seats);
    void ticketBooked(QString ticketNumber, QString status);
    void ticketPaid(QString ticketNumber);
    void ticketCancelled(QString ticketNumber);
    void ticketsReceived(QList<Ticket> tickets);
    void ticketDetailsReceived(Ticket ticket);
    void profileReceived(UserProfile profile);
    void resendVerificationSuccess();
    void passwordChanged();
    void passwordChangeError(QString errorMessage);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    ApiClient();
    ~ApiClient() = default;
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    QTcpSocket* m_socket;
    QByteArray m_buffer;

    bool m_authenticated;
    QString m_sessionToken;
    UserProfile m_userProfile;

    void sendCommand(const QString& command, const QJsonObject& data = QJsonObject());
    void processResponse(const QByteArray& data);

    void handleRegisterResponse(const QJsonObject& response);
    void handleLoginResponse(const QJsonObject& response);
    void handleLogoutResponse(const QJsonObject&);
    void handleVerifyEmailResponse(const QJsonObject&);
    void handleStationsResponse(const QJsonObject& response);
    void handleTrainsResponse(const QJsonObject& response);
    void handleSeatsResponse(const QJsonObject& response);
    void handleBookTicketResponse(const QJsonObject& response);
    void handlePayTicketResponse(const QJsonObject& response);
    void handleCancelTicketResponse(const QJsonObject& response);
    void handleMyTicketsResponse(const QJsonObject& response);
    void handleTicketDetailsResponse(const QJsonObject& response);
    void handleProfileResponse(const QJsonObject& response);
    void handlePasswordChangeResponse(const QJsonObject&);
};
#endif
