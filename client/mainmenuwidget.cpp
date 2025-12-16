#include "mainmenuwidget.h"
#include "ui_mainwindow.h"
#include "loginwidget.h"
#include "mainmenuwidget.h"
#include "trainlistwidget.h"
#include "seatselectionwidget.h"
#include "bookingwidget.h"
#include "myticketswidget.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCompleter>
#include <QStringListModel>

MainMenuWidget::MainMenuWidget(QWidget *parent)
    : QWidget{parent}
{
    setupUi();

    connect(&ApiClient::instance(), &ApiClient::stationsReceived, this, &MainMenuWidget::onStationsReceived);
}

void MainMenuWidget::initialize(){
    loadStations();
}

void MainMenuWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QHBoxLayout* topLayout = new QHBoxLayout();

    QLabel* welcomeLabel = new QLabel("Система бронирования билетов", this);
    QFont welcomeFont = welcomeLabel->font();
    welcomeFont.setPointSize(16);
    welcomeFont.setBold(true);
    welcomeLabel->setFont(welcomeFont);

    m_myTicketsButton = new QPushButton("Мои билеты", this);
    m_myTicketsButton->setMinimumSize(120, 35);
    m_myTicketsButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #1976D2;
        }
    )");

    m_profileButton = new QPushButton("Профиль", this);
    m_profileButton->setMinimumSize(100, 35);

    m_logoutButton = new QPushButton("Выход", this);
    m_logoutButton->setMinimumSize(100, 35);
    m_logoutButton->setStyleSheet(R"(
        QPushButton {
            background-color: #f44336;
            color: white;
            border: none;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #d32f2f;
        }
    )");

    topLayout->addWidget(welcomeLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_myTicketsButton);
    topLayout->addWidget(m_profileButton);
    topLayout->addWidget(m_logoutButton);

    QGroupBox* searchGroup = new QGroupBox("Поиск билетов", this);
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

    QHBoxLayout* citiesLayout = new QHBoxLayout();

    QVBoxLayout* depLayout = new QVBoxLayout();
    QLabel* depLabel = new QLabel("Откуда:", this);
    m_departureInput = new QLineEdit(this);
    m_departureInput->setPlaceholderText("Введите станцию...");
    m_departureInput->setMinimumHeight(35);

    m_departureCompleter = new QCompleter(this);
    m_departureCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_departureCompleter->setFilterMode(Qt::MatchContains);
    m_departureInput->setCompleter(m_departureCompleter);

    depLayout->addWidget(depLabel);
    depLayout->addWidget(m_departureInput);

    QVBoxLayout* arrLayout = new QVBoxLayout();
    QLabel* arrLabel = new QLabel("Куда:", this);
    m_arrivalInput = new QLineEdit(this);
    m_arrivalInput->setPlaceholderText("Введите станцию...");
    m_arrivalInput->setMinimumHeight(35);

    m_arrivalCompleter = new QCompleter(this);
    m_arrivalCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_arrivalCompleter->setFilterMode(Qt::MatchContains);
    m_arrivalInput->setCompleter(m_arrivalCompleter);

    arrLayout->addWidget(arrLabel);
    arrLayout->addWidget(m_arrivalInput);

    QVBoxLayout* dateLayout = new QVBoxLayout();
    QLabel* dateLabel = new QLabel("Дата:", this);
    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setDate(QDate::currentDate());
    m_dateEdit->setMinimumDate(QDate::currentDate());
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setMinimumHeight(35);
    dateLayout->addWidget(dateLabel);
    dateLayout->addWidget(m_dateEdit);

    citiesLayout->addLayout(depLayout, 2);
    citiesLayout->addLayout(arrLayout, 2);
    citiesLayout->addLayout(dateLayout, 1);

    m_searchButton = new QPushButton("Найти билеты", this);
    m_searchButton->setMinimumHeight(45);
    m_searchButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");

    searchLayout->addLayout(citiesLayout);
    searchLayout->addWidget(m_searchButton);

    QGroupBox* popularGroup = new QGroupBox("Популярные направления", this);
    QVBoxLayout* popularLayout = new QVBoxLayout(popularGroup);

    m_popularRoutesList = new QListWidget(this);
    m_popularRoutesList->setMaximumHeight(200);
    setupPopularRoutes();

    popularLayout->addWidget(m_popularRoutesList);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(searchGroup);
    mainLayout->addWidget(popularGroup);
    mainLayout->addStretch();

    connect(m_searchButton, &QPushButton::clicked, this, &MainMenuWidget::onSearchClicked);
    connect(m_popularRoutesList, &QListWidget::itemClicked, this, &MainMenuWidget::onPopularRouteClicked);
    connect(m_myTicketsButton, &QPushButton::clicked, this, &MainMenuWidget::onMyTicketsClicked);
    connect(m_profileButton, &QPushButton::clicked, this, &MainMenuWidget::onProfileClicked);
    connect(m_logoutButton, &QPushButton::clicked, this, &MainMenuWidget::onLogoutClicked);
}

void MainMenuWidget::loadStations()
{
    ApiClient::instance().getStations();
}

void MainMenuWidget::onStationsReceived(QList<Station> stations)
{
    m_stations = stations;

    QStringList stationList;
    QMap<QString, int> stationMap;

    for (const Station& station : stations) {
        QString displayText = QString("%1 (%2)").arg(station.city, station.name);
        stationList << displayText;
        stationMap[displayText] = station.id;
    }

    m_departureCompleter->setModel(new QStringListModel(stationList, m_departureCompleter));
    m_departureStationMap = stationMap;

    m_arrivalCompleter->setModel(new QStringListModel(stationList, m_arrivalCompleter));
    m_arrivalStationMap = stationMap;
}

void MainMenuWidget::setupPopularRoutes()
{
    QStringList popularRoutes = {
        "Москва → Санкт-Петербург",
        "Москва → Казань",
        "Санкт-Петербург → Москва",
        "Екатеринбург → Москва",
        "Новосибирск → Москва",
        "Москва → Владивосток"
    };

    for (const QString& route: popularRoutes) {
        QListWidgetItem* item = new QListWidgetItem(route, m_popularRoutesList);
        item->setSizeHint(QSize(0, 40));
    }
}

void MainMenuWidget::onSearchClicked()
{
    QString depText = m_departureInput->text().trimmed();
    QString arrText = m_arrivalInput->text().trimmed();

    if (depText.isEmpty() || arrText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите станции отправления и прибытия");
        return;
    }

    int depId = m_departureStationMap.value(depText, -1);
    int arrId = m_arrivalStationMap.value(arrText, -1);

    if (depId == -1) {
        QMessageBox::warning(this, "Ошибка", "Станция отправления не найдена");
        return;
    }

    if (arrId == -1) {
        QMessageBox::warning(this, "Ошибка", "Станция прибытия не найдена");
        return;
    }

    if (depId == arrId) {
        QMessageBox::warning(this, "Ошибка", "Станции отправления и прибытия должны различаться");
        return;
    }

    QDate date = m_dateEdit->date();

    emit searchRequested(depId, depText, arrId, arrText, date);
}

void MainMenuWidget::onPopularRouteClicked(QListWidgetItem* item)
{
    if (!item) return;
    QString routeText = item->text();

    QStringList parts = routeText.split(" → ");
    if (parts.size() != 2) {
        return;
    }

    QString departureCity = parts[0].trimmed();
    QString arrivalCity = parts[1].trimmed();

    QString foundDepText;
    QString foundArrText;

    for (const auto& [displayText, stationId] : m_departureStationMap.toStdMap()) {
        if (displayText.startsWith(departureCity)) {
            foundDepText = displayText;
            break;
        }
    }

    for (const auto& [displayText, stationId] : m_arrivalStationMap.toStdMap()) {
        if (displayText.startsWith(arrivalCity)) {
            foundArrText = displayText;
            break;
        }
    }

    if (!foundDepText.isEmpty()) {
        m_departureInput->setText(foundDepText);
    }

    if (!foundArrText.isEmpty()) {
        m_arrivalInput->setText(foundArrText);
    }

    if (!foundDepText.isEmpty() && !foundArrText.isEmpty()) {
        if (m_dateEdit->date() < QDate::currentDate()) {
            m_dateEdit->setDate(QDate::currentDate());
        }
        onSearchClicked();
    } else {
        QString notFound;
        if (foundDepText.isEmpty()) {
            notFound += QString("'%1'").arg(departureCity);
        }
        if (foundArrText.isEmpty()) {
            if (!notFound.isEmpty()) notFound += " и ";
            notFound += QString("'%1'").arg(arrivalCity);
        }

        QMessageBox::warning(this, "Станция не найдена", QString("Станция для города %1 не найдена в списке доступных станций.").arg(notFound));
    }
}

void MainMenuWidget::onMyTicketsClicked()
{
    emit myTicketsRequested();
}

void MainMenuWidget::onProfileClicked()
{
    emit profileRequested();
}

void MainMenuWidget::onLogoutClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Выход", "Вы уверены, что хотите выйти?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        ApiClient::instance().logout();
        emit logoutRequested();
    }
}
