#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "loginwidget.h"
#include "mainmenuwidget.h"
#include "trainlistwidget.h"
#include "seatselectionwidget.h"
#include "bookingwidget.h"
#include "registration.h"
#include "myticketswidget.h"
#include "profilewidget.h"
#include <QVBoxLayout>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupUi();
    createWidgets();
    connectSignals();

    showLogin();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    setWindowTitle("Система бронирования железнодорожных билетов");
    setMinimumSize(1240, 960);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_stackedWidget = new QStackedWidget(centralWidget);
    layout->addWidget(m_stackedWidget);

    setCentralWidget(centralWidget);
}

void MainWindow::createWidgets()
{
    m_loginWidget = new LoginWidget(this);
    m_stackedWidget->addWidget(m_loginWidget);

    m_mainMenuWidget = new MainMenuWidget(this);
    m_stackedWidget->addWidget(m_mainMenuWidget);

    m_trainListWidget = new TrainListWidget(this);
    m_stackedWidget->addWidget(m_trainListWidget);

    m_seatSelectionWidget = new SeatSelectionWidget(this);
    m_stackedWidget->addWidget(m_seatSelectionWidget);

    m_bookingWidget = new BookingWidget(this);
    m_stackedWidget->addWidget(m_bookingWidget);

    m_myTicketsWidget = new MyTicketsWidget(this);
    m_stackedWidget->addWidget(m_myTicketsWidget);

    m_profileWidget = new ProfileWidget(this);
    m_stackedWidget->addWidget(m_profileWidget);
}

void MainWindow::connectSignals()
{
    connect(&ApiClient::instance(), &ApiClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(&ApiClient::instance(), &ApiClient::logoutSuccess, this, &MainWindow::onLogoutSuccess);

    connect(m_loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::showMainMenu);
    connect(m_loginWidget, &LoginWidget::registerRequested, this, &MainWindow:: showRegistration);

    connect(m_mainMenuWidget, &MainMenuWidget::searchRequested, this, &MainWindow::showTrainList);
    connect(m_mainMenuWidget, &MainMenuWidget::myTicketsRequested, this, &MainWindow::showMyTickets);
    connect(m_mainMenuWidget, &MainMenuWidget::logoutRequested, this, &MainWindow::showLogin);

    connect(m_trainListWidget, &TrainListWidget::backRequested, this, &MainWindow::showMainMenu);
    connect(m_trainListWidget, &TrainListWidget::trainSelected, this, &MainWindow::showSeatSelection);

    connect(m_seatSelectionWidget, &SeatSelectionWidget::backRequested,
            [this]() { m_stackedWidget->setCurrentWidget(m_trainListWidget); });
    connect(m_seatSelectionWidget, &SeatSelectionWidget::seatSelected, this, &MainWindow::showBooking);

    connect(m_bookingWidget, &BookingWidget::backRequested,
            [this]() { m_stackedWidget->setCurrentWidget(m_seatSelectionWidget); });
    connect(m_bookingWidget, &BookingWidget::bookingCompleted, this, &MainWindow::onBookingCompleted);

    connect(m_myTicketsWidget, &MyTicketsWidget::backRequested, this, &MainWindow::showMainMenu);

    connect(m_profileWidget, &ProfileWidget::backRequested, this, &MainWindow::showMainMenu);
    connect(m_mainMenuWidget, &MainMenuWidget::profileRequested, this, &MainWindow::showProfile);
}

void MainWindow::showLogin()
{
    m_stackedWidget->setCurrentWidget(m_loginWidget);
}

void MainWindow::showMainMenu()
{
    m_stackedWidget->setCurrentWidget(m_mainMenuWidget);
}

void MainWindow::showTrainList(int depId, QString depName, int arrId, QString arrName, QDate date)
{
    m_trainListWidget->searchTrains(depId, depName, arrId, arrName, date);
    m_stackedWidget->setCurrentWidget(m_trainListWidget);
}

void MainWindow::showSeatSelection(TrainSearchResult train, int depId, int arrId)
{
    m_seatSelectionWidget->loadSeats(train, depId, arrId);
    m_stackedWidget->setCurrentWidget(m_seatSelectionWidget);
}

void MainWindow::showBooking(TrainSearchResult train, Seat seat, int depId, int arrId, double price)
{
    m_bookingWidget->setBookingData(train, seat, depId, arrId, price);
    m_stackedWidget->setCurrentWidget(m_bookingWidget);
}

void MainWindow::showMyTickets()
{
    m_myTicketsWidget->loadTickets();
    m_stackedWidget->setCurrentWidget(m_myTicketsWidget);
}

void MainWindow::showProfile()
{
    m_profileWidget->loadProfile();
    m_stackedWidget->setCurrentWidget(m_profileWidget);
}

void MainWindow::onLoginSuccess(UserProfile user)
{
    QString welcomeMsg = QString("Добро пожаловать, %1!").arg(user.name);
    statusBar()->showMessage(welcomeMsg, 5000);
    m_mainMenuWidget->initialize();
    m_stackedWidget->setCurrentWidget(m_mainMenuWidget);
}

void MainWindow::onLogoutSuccess()
{
    statusBar()->showMessage("Вы вышли из системы", 3000);
    showLogin();
}

void MainWindow::onBookingCompleted()
{
    showMainMenu();
    QMessageBox::information(this, "Успешно", "Бронирование завершено!\nВы можете просмотреть свои билеты в разделе 'Мои билеты'.");
}

void MainWindow::showRegistration(){
    Registration* reg = new Registration();
    reg->setAttribute(Qt:: WA_DeleteOnClose);
    reg->show();
}
