#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include <QTextStream>
#include "apiserver.h"
#include "database.h"

void printBanner()
{
    QTextStream out(stdout);
    out << "========================================\n";
    out << "  Railway Ticket Booking Server\n";
    out << "  Version 1.0.0\n";
    out << "========================================\n";
    out << "\n";
}

void printServerInfo(quint16 port, const QString& host)
{
    QTextStream out(stdout);
    out << "Server Information:\n";
    out << "  Host: " << host << "\n";
    out << "  Port: " << port << "\n";
    out << "  Started at: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << "\n";
    out << "Server is running. Press Ctrl+C to stop.\n";
    out << "========================================\n";
    out.flush();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Railway Ticket Booking Server");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Railway Inc.");

    printBanner();

    qDebug() << "Connecting to database...";
    if (!Database::instance().connectFromConfig("config/database.conf")) {
        qCritical() << "Failed to connect to database!";
        qCritical() << "Error:" << Database::instance().lastError();
        qCritical() << "Please check config/database.conf file.";
        return 1;
    }

    qDebug() << "Database connected successfully!";
    qDebug() << "";
    ApiServer server;

    QString host = "0.0.0.0";
    quint16 port = 8080;

    QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--port" || args[i] == "-p") {
            if (i + 1 < args.size()) {
                port = args[++i].toUShort();
            }
        } else if (args[i] == "--host" || args[i] == "-h") {
            if (i + 1 < args.size()) {
                host = args[++i];
            }
        } else if (args[i] == "--help") {
            QTextStream out(stdout);
            out << "Usage: " << args[0] << " [OPTIONS]\n";
            out << "\n";
            out << "Options:\n";
            out << "  -p, --port PORT    Set server port (default: 8080)\n";
            out << "  -h, --host HOST    Set server host (default: 0.0.0.0)\n";
            out << "  --help             Show this help message\n";
            out << "\n";
            out << "Examples:\n";
            out << "  " << args[0] << " --port 9090\n";
            out << "  " << args[0] << " --host 127.0.0.1 --port 8080\n";
            out.flush();
            return 0;
        }
    }

    server.setMaxConnections(100);
    server.setConnectionTimeout(300);
    server.setBookingTimeout(15);
    if (!server.startServer(port, host)) {
        qCritical() << "Failed to start server!";
        qCritical() << "Make sure port" << port << "is not already in use.";
        return 1;
    }

    printServerInfo(port, host);

    QObject::connect(&server, &ApiServer::clientConnected, [](QString address, quint16 port) {
                         qDebug() << "New client connected:" << address << ":" << port;
                     });

    QObject::connect(&server, &ApiServer::clientDisconnected, [](QString address) {
                         qDebug() << "Client disconnected:" << address;
                     });

    QObject::connect(&server, &ApiServer::errorOccured, [](QString error) {
                         qWarning() << "Server error:" << error;
                     });

    QObject::connect(&server, &ApiServer::authenticationSuccess, [](int userId, QString email) {
                         qDebug() << "User authenticated:" << email << "(ID:" << userId << ")";
                     });

    int result = app.exec();

    qDebug() << "\nShutting down server...";
    server.stopServer();

    qDebug() << "Server stopped.";
    qDebug() << "Goodbye!";

    return result;
}
