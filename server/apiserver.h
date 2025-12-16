#ifndef APISERVER_H
#define APISERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSslSocket>
#include <QMap>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include "database.h"

class ClientHandler;

class ApiServer : public QObject
{
    Q_OBJECT

public:
    explicit ApiServer(QObject* parent = nullptr);
    ~ApiServer();

    bool startServer(quint16 port = 8080, const QString& address = "0.0.0.0");
    void stopServer();
    bool isRunning() const;

    bool enableSSL(const QString& certPath, const QString& keyPath);
    bool isSSLEnabled() const;

    struct ServerStats{
        int activeConnections;
        int totalConnections;
        quint64 bytesRecieved;
        quint64 bytesSent;
        QDateTime startTime;
        int authenticatedUsers;
    };
    ServerStats getStatistics() const;

    void setMaxConnections(int max);
    void setConnectionTimeout(int seconds);
    void setBookingTimeout(int minutes);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(QString address, quint16 port);
    void clientDisconnected(QString address);
    void errorOccured(QString error);
    void messageReceived(QString from, QString message);
    void authenticationSuccess(int userId, QString email);
    void authenticationFailed(QString email, QString reason);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientError(QAbstractSocket::SocketError error);
    void onCleanupTimer();

private:
    QTcpServer* m_server;
    QMap<QTcpSocket*, ClientHandler*> m_clients;
    QMutex m_mutex;

    bool m_sslEnabled;
    QString m_certPath;
    QString m_keyPath;

    int m_maxConnections;
    int m_connectionTimeout;
    int m_bookingTimeout;

    ServerStats m_stats;
    QTimer* m_cleanup_Timer;

    void removeClient(QTcpSocket* socket);
    void broadcastMessage(const QJsonObject& message, QTcpSocket* exclude = nullptr);
};

class ClientHandler : public QObject{
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket* socket, QObject* parent = nullptr);
    ~ClientHandler();

    QString getAddress() const;
    quint16 getPort() const;
    bool isAuthenticated() const;
    int getUserId() const;
    QString getSessionToken() const;

    void sendResponse(const QJsonObject& response);
    void sendError(const QString& error, const QString& command = "");

signals:
    void disconnected();
    void errorOccurred(QAbstractSocket::SocketError error);
    void authenticated(int userId, QString email);
    void commandExecuted(QString command, bool success);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;

    bool m_authenticated;
    int m_userId;
    QString m_userEmail;
    QString m_sessionToken;

    void processMessage(const QByteArray& data);
    void handleCommand(const QJsonObject& request);
    void sendVerificationEmail(const QString& recipientEmail, const QString& code);
    void sendTicketEmail(const QString& recipientEmail
                        , const Ticket& ticket
                        , const QByteArray& pdfData);

    void handleRegister(const QJsonObject& data);
    void handleLogin(const QJsonObject& data);
    void handleLogout();
    void handleResendVerification(const QJsonObject& data);
    void handleVerifyEmail(const QJsonObject& data);
    void handleSearchTrains(const QJsonObject& data);
    void handleGetStations(const QJsonObject& data);
    void handleGetAvailableSeats(const QJsonObject& data);
    void handleBookTicket(const QJsonObject& data);
    void handlePayTicket(const QJsonObject& data);
    void handleCancelTicket(const QJsonObject& data);
    void handleGetMyTickets();
    void handleGetTicketDetails(const QJsonObject& data);
    void handleChangePassword(const QJsonObject& data);
    void handleGetProfile();

    bool requireAuth(const QString& command);
    QJsonObject createResponse(const QString& command
                               , bool success
                               , const QString& message = ""
                               , const QJsonObject& data = QJsonObject());
    QJsonObject userToJson(const User& user);
    QJsonObject stationToJson(const Station& station);
    QJsonObject ticketToJson(const Ticket& ticket);
    QJsonObject searchResultToJson(const Database::SearchResult& result);
};
#endif
