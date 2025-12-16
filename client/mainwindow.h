#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "apiclient.h"

class LoginWidget;
class MainMenuWidget;
class TrainListWidget;
class SeatSelectionWidget;
class BookingWidget;
class MyTicketsWidget;
class ProfileWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showLogin();
    void showRegistration();
    void showMainMenu();
    void showTrainList(int depId, QString depName, int arrId, QString arrName, QDate date);
    void showSeatSelection(TrainSearchResult train, int depId, int arrId);
    void showBooking(TrainSearchResult train, Seat seat, int depId, int arrId, double price);
    void showMyTickets();
    void showProfile();

    void onLoginSuccess(UserProfile user);
    void onLogoutSuccess();
    void onBookingCompleted();

private:
    Ui::MainWindow *ui;

    QStackedWidget* m_stackedWidget;

    LoginWidget* m_loginWidget;
    MainMenuWidget* m_mainMenuWidget;
    TrainListWidget* m_trainListWidget;
    SeatSelectionWidget* m_seatSelectionWidget;
    BookingWidget* m_bookingWidget;
    MyTicketsWidget* m_myTicketsWidget;
    ProfileWidget* m_profileWidget;

    void setupUi();
    void createWidgets();
    void connectSignals();
};

#endif // MAINWINDOW_H
