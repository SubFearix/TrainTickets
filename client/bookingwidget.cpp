#include "bookingwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QRegularExpression>

BookingWidget::BookingWidget(QWidget *parent)
    : QWidget(parent)
    , m_timeoutMinutes(15)
{
    setupUi();

    m_bookingTimer = new QTimer(this);
    connect(m_bookingTimer, &QTimer::timeout, this, &BookingWidget::updateTimer);

    connect(&ApiClient::instance(), &ApiClient::ticketBooked, this, &BookingWidget::onTicketBooked);
    connect(&ApiClient::instance(), &ApiClient::ticketPaid, this, &BookingWidget::onTicketPaid);
    connect(&ApiClient::instance(), &ApiClient::error, this, &BookingWidget::onError);
}

void BookingWidget::setupUi(){
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

    QLabel* titleLabel = new QLabel("Бронирование билета", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    headerLayout->addWidget(m_backButton);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QGroupBox* tripInfoGroup = new QGroupBox("Информация о поездке", this);
    QVBoxLayout* tripInfoLayout = new QVBoxLayout(tripInfoGroup);

    m_tripInfoLabel = new QLabel(this);
    m_seatInfoLabel = new QLabel(this);
    m_priceLabel = new QLabel(this);
    QFont priceFont = m_priceLabel->font();
    priceFont.setPointSize(16);
    priceFont.setBold(true);
    m_priceLabel->setFont(priceFont);
    m_priceLabel->setStyleSheet("color: #2196F3;");

    tripInfoLayout->addWidget(m_tripInfoLabel);
    tripInfoLayout->addWidget(m_seatInfoLabel);
    tripInfoLayout->addWidget(m_priceLabel);

    m_bookingFormWidget = new QWidget(this);
    QVBoxLayout* bookingFormLayout = new QVBoxLayout(m_bookingFormWidget);

    QGroupBox* passengerGroup = new QGroupBox("Данные пассажира", this);
    QFormLayout* formLayout = new QFormLayout(passengerGroup);

    m_useMyselfCheckbox = new QCheckBox("Еду я", this);
    m_useMyselfCheckbox->setStyleSheet("font-weight: bold;");

    m_passengerNameEdit = new QLineEdit(this);
    m_passengerNameEdit->setPlaceholderText("Фамилия Имя Отчество");
    m_passengerNameEdit->setMinimumHeight(35);

    m_passengerDocumentEdit = new QLineEdit(this);
    m_passengerDocumentEdit->setPlaceholderText("Серия и номер паспорта (например: 1234 567890)");
    m_passengerDocumentEdit->setMinimumHeight(35);

    formLayout->addRow(m_useMyselfCheckbox);
    formLayout->addRow("ФИО:", m_passengerNameEdit);
    formLayout->addRow("Документ:", m_passengerDocumentEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: red;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_bookButton = new QPushButton("Забронировать", this);
    m_bookButton->setMinimumHeight(45);
    m_bookButton->setStyleSheet(R"(
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

    bookingFormLayout->addWidget(passengerGroup);
    bookingFormLayout->addWidget(m_errorLabel);
    bookingFormLayout->addWidget(m_bookButton);

    m_paymentFormWidget = new QWidget(this);
    QVBoxLayout* paymentLayout = new QVBoxLayout(m_paymentFormWidget);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("font-size: 14px;");

    m_timerLabel = new QLabel(this);
    QFont timerFont = m_timerLabel->font();
    timerFont.setPointSize(18);
    timerFont.setBold(true);
    m_timerLabel->setFont(timerFont);
    m_timerLabel->setStyleSheet("color: #f44336;");
    m_timerLabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout* paymentButtonsLayout = new QHBoxLayout();

    m_payButton = new QPushButton("Оплатить", this);
    m_payButton->setMinimumSize(150, 45);
    m_payButton->setStyleSheet(R"(
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

    m_cancelBookingButton = new QPushButton("Отменить бронь", this);
    m_cancelBookingButton->setMinimumSize(150, 45);
    m_cancelBookingButton->setStyleSheet(R"(
        QPushButton {
            background-color: #f44336;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #d32f2f;
        }
    )");

    paymentButtonsLayout->addStretch();
    paymentButtonsLayout->addWidget(m_payButton);
    paymentButtonsLayout->addWidget(m_cancelBookingButton);
    paymentButtonsLayout->addStretch();

    paymentLayout->addWidget(m_statusLabel);
    paymentLayout->addSpacing(20);
    paymentLayout->addWidget(m_timerLabel);
    paymentLayout->addSpacing(20);
    paymentLayout->addLayout(paymentButtonsLayout);

    m_paymentFormWidget->hide();

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(tripInfoGroup);
    mainLayout->addWidget(m_bookingFormWidget);
    mainLayout->addWidget(m_paymentFormWidget);
    mainLayout->addStretch();

    connect(m_backButton, &QPushButton::clicked, this, &BookingWidget::backRequested);
    connect(m_useMyselfCheckbox, &QCheckBox::toggled, this, &BookingWidget::onUseMyselfClicked);
    connect(m_bookButton, &QPushButton::clicked, this, &BookingWidget::onBookClicked);
    connect(m_payButton, &QPushButton::clicked, this, &BookingWidget::onPayClicked);
    connect(m_cancelBookingButton, &QPushButton::clicked, this, &BookingWidget::onCancelBookingClicked);
}

void BookingWidget::setBookingData(const TrainSearchResult& train, const Seat& seat, int depId, int arrId, double price){
    m_train = train;
    m_seat = seat;
    m_departureStationId = depId;
    m_arrivalStationId = arrId;
    m_price = price;

    resetForm();

    m_tripInfoLabel->setText(QString("Поезд №%1 (%2)\n%3 → %4\nОтправление: %5")
                                 .arg(train.trainNumber)
                                 .arg(train.trainType)
                                 .arg(train.departureStationName)
                                 .arg(train.arrivalStationName)
                                 .arg(train.departureTime.toString("dd.MM.yyyy HH:mm")));

    m_seatInfoLabel->setText(QString("Место №%1 (%2)")
                                 .arg(seat.seatNumber)
                                 .arg(seat.seatType));

    m_priceLabel->setText(QString("Цена: %1 ₽").arg(price, 0, 'f', 2));
}

void BookingWidget::onUseMyselfClicked(bool checked){
    if (checked) {
        UserProfile profile = ApiClient::instance().getUserProfile();
        m_passengerNameEdit->setText(QString("%1 %2").arg(profile.surname, profile.name));
    } else {
        m_passengerNameEdit->clear();
        m_passengerDocumentEdit->clear();
    }
}

bool BookingWidget::validateForm()
{
    QString name = m_passengerNameEdit->text().trimmed();
    QString document = m_passengerDocumentEdit->text().trimmed();

    if (name.isEmpty()) {
        m_errorLabel->setText("Введите ФИО пассажира");
        m_errorLabel->show();
        return false;
    }

    if (document.isEmpty()) {
        m_errorLabel->setText("Введите номер документа");
        m_errorLabel->show();
        return false;
    }

    QString cleanDoc = document;
    cleanDoc.replace(" ", "").replace("-", "");

    QRegularExpression docRegex("^\\d{10}$");
    if (!docRegex.match(cleanDoc).hasMatch()) {
        m_errorLabel->setText("Неверный формат документа. Введите 10 цифр (например: 1234 567890)");
        m_errorLabel->show();
        return false;
    }

    return true;
}

void BookingWidget::onBookClicked(){
    m_errorLabel->hide();

    if (!validateForm()) {
        return;
    }

    m_bookButton->setEnabled(false);
    m_bookButton->setText("Бронирование...");

    QString passengerName = m_passengerNameEdit->text().trimmed();
    QString passengerDocument = m_passengerDocumentEdit->text().trimmed();
    passengerDocument.replace(" ", "").replace("-", "");

    ApiClient::instance().bookTicket(
        m_train.scheduleId,
        m_seat.id,
        m_departureStationId,
        m_arrivalStationId,
        passengerName,
        passengerDocument,
        m_price
        );
}

void BookingWidget::onTicketBooked(QString ticketNumber, QString status){
    m_bookButton->setEnabled(true);
    m_bookButton->setText("Забронировать");

    m_currentTicketNumber = ticketNumber;

    m_statusLabel->setText(QString("✅ Билет успешно забронирован!\n"
                                   "Номер билета: %1\n\n"
                                   "⏰ Пожалуйста, оплатите билет в течение 15 минут.")
                               .arg(ticketNumber));

    m_bookingTime = QDateTime::currentDateTime();
    m_bookingTimer->start(1000);

    showPaymentForm();
}

void BookingWidget::updateTimer(){
    QDateTime now = QDateTime::currentDateTime();
    int secondsElapsed = m_bookingTime.secsTo(now);
    int secondsRemaining = (m_timeoutMinutes * 60) - secondsElapsed;

    if (secondsRemaining <= 0) {
        m_bookingTimer->stop();
        m_timerLabel->setText("⏰ Время истекло!");
        m_payButton->setEnabled(false);

        QMessageBox::warning(this, "Время истекло", "Время бронирования истекло. Бронь была автоматически отменена.");
        resetForm();
        return;
    }

    int minutes = secondsRemaining / 60;
    int seconds = secondsRemaining % 60;

    QString timerText = QString("⏰ Осталось времени: %1:%2")
                            .arg(minutes, 2, 10, QChar('0'))
                            .arg(seconds, 2, 10, QChar('0'));

    if (secondsRemaining <= 60) {
        m_timerLabel->setStyleSheet("color: #f44336; font-weight: bold;");
    } else if (secondsRemaining <= 180) {
        m_timerLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
    }

    m_timerLabel->setText(timerText);
}

void BookingWidget::onPayClicked(){
    m_payButton->setEnabled(false);
    m_payButton->setText("Оплата...");

    ApiClient::instance().payTicket(m_currentTicketNumber);
}

void BookingWidget::onTicketPaid(QString ticketNumber){
    m_bookingTimer->stop();

    QMessageBox::information(this, "✅ Успешно",
                             QString("Билет %1 успешно оплачен!\n\n"
                                     "📧 Билет отправлен на вашу электронную почту в формате PDF.\n\n"
                                     "Вы можете просмотреть билет в разделе 'Мои билеты'.")
                                 .arg(ticketNumber));

    emit bookingCompleted();
}

void BookingWidget::onCancelBookingClicked(){
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Отмена брони",
        "Вы уверены, что хотите отменить бронирование?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        m_bookingTimer->stop();
        ApiClient::instance().cancelTicket(m_currentTicketNumber, "Отмена пользователем");

        QMessageBox::information(this, "Отменено", "Бронирование отменено");

        emit backRequested();
    }
}

void BookingWidget::onError(QString errorMessage){
    m_bookButton->setEnabled(true);
    m_bookButton->setText("Забронировать");
    m_payButton->setEnabled(true);
    m_payButton->setText("Оплатить");

    m_errorLabel->setText(errorMessage);
    m_errorLabel->show();
}

void BookingWidget::showBookingForm(){
    m_bookingFormWidget->show();
    m_paymentFormWidget->hide();
}

void BookingWidget::showPaymentForm(){
    m_bookingFormWidget->hide();
    m_paymentFormWidget->show();
}

void BookingWidget::resetForm(){
    m_passengerNameEdit->clear();
    m_passengerDocumentEdit->clear();
    m_useMyselfCheckbox->setChecked(false);
    m_errorLabel->hide();
    m_currentTicketNumber.clear();
    m_bookingTimer->stop();

    m_bookButton->setEnabled(true);
    m_bookButton->setText("Забронировать");
    m_payButton->setEnabled(true);
    m_payButton->setText("Оплатить");

    m_timerLabel->setStyleSheet("color: #f44336; font-weight: bold;");

    showBookingForm();
}
