#include "myticketswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFrame>

MyTicketsWidget::MyTicketsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&ApiClient::instance(), &ApiClient::ticketsReceived, this, &MyTicketsWidget::onTicketsReceived);
}

void MyTicketsWidget::setupUi(){
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QHBoxLayout* headerLayout = new QHBoxLayout();

    m_backButton = new QPushButton("← Назад", this);
    m_backButton->setMaximumWidth(100);
    m_backButton->setStyleSheet(R"(
        QPushButton {
            background-color: #757575;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 15px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #616161;
        }
    )");

    m_titleLabel = new QLabel("Мои билеты", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_refreshButton = new QPushButton("🔄 Обновить", this);
    m_refreshButton->setMaximumWidth(120);
    m_refreshButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 15px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #1976D2;
        }
    )");

    headerLayout->addWidget(m_backButton);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshButton);

    // Табы для активных и завершенных билетов
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #ddd;
            border-radius: 5px;
            background: white;
        }
        QTabBar::tab {
            background: #f5f5f5;
            border: 1px solid #ddd;
            padding: 10px 20px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: white;
            border-bottom-color: white;
        }
    )");

    m_activeTicketsList = new QListWidget(this);
    m_activeTicketsList->setStyleSheet(R"(
        QListWidget {
            border: none;
            background-color: white;
        }
        QListWidget::item {
            border-bottom: 1px solid #eee;
            padding: 5px;
        }
        QListWidget::item:hover {
            background-color: #f5f5f5;
        }
        QListWidget::item:selected {
            background-color: #E3F2FD;
            color: black;
        }
    )");

    m_historyTicketsList = new QListWidget(this);
    m_historyTicketsList->setStyleSheet(m_activeTicketsList->styleSheet());

    m_tabWidget->addTab(m_activeTicketsList, "Активные");
    m_tabWidget->addTab(m_historyTicketsList, "История");

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_tabWidget);

    connect(m_backButton, &QPushButton::clicked, this, &MyTicketsWidget::onBackClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &MyTicketsWidget::onRefreshClicked);
    connect(m_activeTicketsList, &QListWidget::itemClicked, this, &MyTicketsWidget::onTicketItemClicked);
    connect(m_historyTicketsList, &QListWidget::itemClicked, this, &MyTicketsWidget::onTicketItemClicked);
}

void MyTicketsWidget::loadTickets(){
    m_activeTicketsList->clear();
    m_historyTicketsList->clear();

    QListWidgetItem* loadingItem1 = new QListWidgetItem("Загрузка билетов...");
    loadingItem1->setTextAlignment(Qt::AlignCenter);
    loadingItem1->setFlags(Qt::NoItemFlags);
    m_activeTicketsList->addItem(loadingItem1);

    QListWidgetItem* loadingItem2 = new QListWidgetItem("Загрузка билетов...");
    loadingItem2->setTextAlignment(Qt::AlignCenter);
    loadingItem2->setFlags(Qt::NoItemFlags);
    m_historyTicketsList->addItem(loadingItem2);

    ApiClient::instance().getMyTickets();
}

void MyTicketsWidget::onTicketsReceived(QList<Ticket> tickets){
    m_tickets = tickets;
    displayTickets();
}

void MyTicketsWidget::displayTickets(){
    m_activeTicketsList->clear();
    m_historyTicketsList->clear();

    if (m_tickets.isEmpty()) {
        QListWidgetItem* noTickets1 = new QListWidgetItem("У вас пока нет билетов");
        noTickets1->setTextAlignment(Qt::AlignCenter);
        noTickets1->setFlags(Qt::NoItemFlags);
        m_activeTicketsList->addItem(noTickets1);

        QListWidgetItem* noTickets2 = new QListWidgetItem("История пуста");
        noTickets2->setTextAlignment(Qt::AlignCenter);
        noTickets2->setFlags(Qt::NoItemFlags);
        m_historyTicketsList->addItem(noTickets2);
        return;
    }

    for (const Ticket& ticket: m_tickets) {
        QWidget* ticketWidget = new QWidget();
        QVBoxLayout* ticketLayout = new QVBoxLayout(ticketWidget);
        ticketLayout->setContentsMargins(10, 10, 10, 10);
        ticketLayout->setSpacing(5);

        QHBoxLayout* topLine = new QHBoxLayout();

        QLabel* ticketNumberLabel = new QLabel(QString("Билет №%1").arg(ticket.ticketNumber));
        QFont boldFont = ticketNumberLabel->font();
        boldFont.setBold(true);
        boldFont.setPointSize(12);
        ticketNumberLabel->setFont(boldFont);

        QLabel* statusLabel = new QLabel(getStatusText(ticket.status));
        statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(getStatusColor(ticket.status)));

        topLine->addWidget(ticketNumberLabel);
        topLine->addStretch();
        topLine->addWidget(statusLabel);

        QLabel* passengerLabel = new QLabel(QString("Пассажир: %1").arg(ticket.passengerName));

        QLabel* dateLabel = new QLabel(QString("Забронировано: %1").arg(ticket.bookedAt.toString("dd.MM.yyyy HH:mm")));
        dateLabel->setStyleSheet("color: #666; font-size: 11px;");

        QLabel* priceLabel = new QLabel(QString("Цена: %1 ₽").arg(ticket.price, 0, 'f', 2));
        QFont priceFont = priceLabel->font();
        priceFont.setBold(true);
        priceFont.setPointSize(13);
        priceLabel->setFont(priceFont);
        priceLabel->setStyleSheet("color: #2196F3;");

        ticketLayout->addLayout(topLine);
        ticketLayout->addWidget(passengerLabel);
        ticketLayout->addWidget(dateLabel);
        ticketLayout->addWidget(priceLabel);

        QListWidget* targetList = nullptr;
        if (ticket.status == "booked" || ticket.status == "paid") {
            targetList = m_activeTicketsList;
        } else {
            targetList = m_historyTicketsList;
        }

        QListWidgetItem* item = new QListWidgetItem(targetList);
        item->setSizeHint(ticketWidget->sizeHint() + QSize(0, 10));
        item->setData(Qt::UserRole, ticket.ticketNumber);
        targetList->addItem(item);
        targetList->setItemWidget(item, ticketWidget);
    }

    if (m_activeTicketsList->count() == 0) {
        QListWidgetItem* noActive = new QListWidgetItem("Нет активных билетов");
        noActive->setTextAlignment(Qt::AlignCenter);
        noActive->setFlags(Qt::NoItemFlags);
        m_activeTicketsList->addItem(noActive);
    }

    if (m_historyTicketsList->count() == 0) {
        QListWidgetItem* noHistory = new QListWidgetItem("История пуста");
        noHistory->setTextAlignment(Qt::AlignCenter);
        noHistory->setFlags(Qt::NoItemFlags);
        m_historyTicketsList->addItem(noHistory);
    }
}

void MyTicketsWidget::onTicketItemClicked(QListWidgetItem* item){
    QString ticketNumber = item->data(Qt::UserRole).toString();
    if (ticketNumber.isEmpty()) return;
    Ticket selectedTicket;
    for (const Ticket& ticket: m_tickets) {
        if (ticket.ticketNumber == ticketNumber) {
            selectedTicket = ticket;
            break;
        }
    }

    if (selectedTicket.id <= 0) return;

    m_selectedTicket = selectedTicket;

    QString detailsText = QString(
                              "Билет №%1\n\n"
                              "Пассажир: %2\n"
                              "Документ: %3\n"
                              "Статус: %4\n"
                              "Цена: %5 ₽\n\n"
                              "Забронировано: %6"
                              ).arg(selectedTicket.ticketNumber)
                              .arg(selectedTicket.passengerName)
                              .arg(selectedTicket.passengerDocument)
                              .arg(getStatusText(selectedTicket.status))
                              .arg(selectedTicket.price, 0, 'f', 2)
                              .arg(selectedTicket.bookedAt.toString("dd.MM.yyyy HH:mm"));

    if (selectedTicket.paidAt.isValid()) {
        detailsText += QString("\nОплачено: %1").arg(selectedTicket.paidAt.toString("dd.MM.yyyy HH:mm"));
    }

    if (selectedTicket.cancelledAt.isValid()) {
        detailsText += QString("\nОтменено: %1").arg(selectedTicket.cancelledAt.toString("dd.MM.yyyy HH:mm"));
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Детали билета");
    msgBox.setText(detailsText);
    msgBox.setIcon(QMessageBox::Information);

    if (selectedTicket.status == "booked") {
        QPushButton* cancelButton = msgBox.addButton("Отменить билет", QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();

        if (msgBox.clickedButton() == cancelButton) {
            onCancelTicketClicked();
        }
    } else {
        msgBox.exec();
    }
}

void MyTicketsWidget::onCancelTicketClicked(){
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Отмена билета",
        QString("Вы уверены, что хотите отменить билет %1?").arg(m_selectedTicket.ticketNumber),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        ApiClient::instance().cancelTicket(m_selectedTicket.ticketNumber, "Отмена пользователем");

        QMessageBox::information(this, "Успешно", "Билет отменен");
        loadTickets();
    }
}

void MyTicketsWidget::onBackClicked(){
    emit backRequested();
}

void MyTicketsWidget::onRefreshClicked(){
    loadTickets();
}

QString MyTicketsWidget::getStatusText(const QString& status) const{
    if (status == "booked") return "Забронирован";
    if (status == "paid") return "Оплачен";
    if (status == "cancelled") return "Отменен";
    if (status == "expired") return "Истек";
    return status;
}

QString MyTicketsWidget::getStatusColor(const QString& status) const{
    if (status == "booked") return "#FF9800";
    if (status == "paid") return "#4CAF50";
    if (status == "cancelled") return "#f44336";
    if (status == "expired") return "#999";
    return "#000";
}
