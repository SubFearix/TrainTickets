#include "mainwindow.h"
#include "apiclient.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QObject::connect(&ApiClient::instance(), &ApiClient::connected, []() {
        qDebug() << "Подключено к серверу";
    });

    QObject::connect(&ApiClient::instance(), &ApiClient::error, [](QString msg) {
        qWarning() << "Ошибка:" << msg;
        QMessageBox::warning(nullptr, "Ошибка подключения", "Не удалось подключиться к серверу.\n" + msg);
    });

    ApiClient::instance().connectToServer("127.0.0.1", 8080);

    MainWindow mainWindow;
    mainWindow.show();

    return a.exec();
}
