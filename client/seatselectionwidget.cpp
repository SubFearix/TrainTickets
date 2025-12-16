#include "seatselectionwidget.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QListView>

SeatSelectionWidget::SeatSelectionWidget(QWidget *parent)
    : QWidget{parent}
    , m_selectedButton(nullptr)
{
    setupUi();
    connect(&ApiClient::instance(), &ApiClient::seatsReceived, this, &SeatSelectionWidget::onSeatsReceived);
}

void SeatSelectionWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    QHBoxLayout* headerLayout = new QHBoxLayout();

    m_backButton = new QPushButton("← Назад", this);
    m_backButton->setFixedWidth(100 );
    m_backButton->setStyleSheet(R"(
        QPushButton {
            background-color: #757575;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 10px 15px;
            font-size:  13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #616161;
        }
    )");

    m_trainInfoLabel = new QLabel(this);
    m_trainInfoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");

    headerLayout->addWidget(m_backButton);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(m_trainInfoLabel);
    headerLayout->addStretch();

    QHBoxLayout* selectorLayout = new QHBoxLayout();

    QLabel* carriageLabel = new QLabel("Вагон:", this);
    carriageLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    m_carriageSelector = new QComboBox(this);
    m_carriageSelector->setMinimumWidth(300);
    m_carriageSelector->setView(new QListView());
    m_carriageSelector->setStyleSheet(R"(
        QComboBox {
            padding: 8px 15px;
            padding-right: 40px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 14px;
            background: white;
        }
        QComboBox:hover {
            border-color: #2196F3;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: center right;
            width: 35px;
            border: none;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: none;
            border: none;
            width: 0;
            height: 0;
        }
        QComboBox QAbstractItemView {
            background-color: white;
            border: 1px solid #ddd;
            selection-background-color: #e3f2fd;
            selection-color: #1976d2;
            outline: none;
        }
        QComboBox QAbstractItemView::item {
            padding: 8px;
            border: none;
            min-height: 25px;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #f5f5f5;
            color: #424242;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #e3f2fd;
            color: #1976d2;
        }
    )");

    QLabel* arrowLabel = new QLabel("▼", m_carriageSelector);
    arrowLabel->setStyleSheet("color: #666; font-size: 12px; background: transparent;");
    arrowLabel->setGeometry(m_carriageSelector->width() - 25, 0, 20, m_carriageSelector->height());
    arrowLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    selectorLayout->addWidget(carriageLabel);
    selectorLayout->addWidget(m_carriageSelector);
    selectorLayout->addStretch();

    QHBoxLayout* legendLayout = new QHBoxLayout();
    legendLayout->setSpacing(20);

    auto addLegendItem = [&](const QString& color, const QString& borderColor, const QString& text) {
        QFrame* frame = new QFrame(this);
        QHBoxLayout* itemLayout = new QHBoxLayout(frame);
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(5);

        QLabel* colorBox = new QLabel(frame);
        colorBox->setFixedSize(24, 20);
        colorBox->setStyleSheet(QString("background-color: %1; border: 2px solid %2; border-radius: 3px;").arg(color, borderColor));

        QLabel* textLabel = new QLabel(text, frame);
        textLabel->setStyleSheet("font-size: 12px; color: #666;");

        itemLayout->addWidget(colorBox);
        itemLayout->addWidget(textLabel);
        legendLayout->addWidget(frame);
    };

    addLegendItem("#e3f2fd", "#90caf9", "Нижнее");
    addLegendItem("#f3e5f5", "#ce93d8", "Верхнее");
    addLegendItem("#e8f5e9", "#a5d6a7", "Боковое ↓");
    addLegendItem("#fff3e0", "#ffcc80", "Боковое ↑");
    addLegendItem("#ffcdd2", "#ef9a9a", "Занято");
    addLegendItem("#ef5350", "#c62828", "Забронировано");
    addLegendItem("#f44336", "#c62828", "Выбрано");

    legendLayout->addStretch();

    m_selectedSeatLabel = new QLabel("Выберите место", this);
    m_selectedSeatLabel->setAlignment(Qt::AlignCenter);
    m_selectedSeatLabel->setStyleSheet(R"(
        QLabel {
            background-color: #f5f5f5;
            border: 1px solid #e0e0e0;
            border-radius: 5px;
            padding: 12px;
            font-size:  14px;
            color: #666;
        }
    )");

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea {
            border: 1px solid #e0e0e0;
            border-radius: 5px;
            background-color: white;
        }
    )");

    m_scrollContent = new QWidget();
    m_carriagesLayout = new QVBoxLayout(m_scrollContent);
    m_carriagesLayout->setSpacing(20);
    m_carriagesLayout->setContentsMargins(15, 15, 15, 15);
    m_scrollArea->setWidget(m_scrollContent);

    m_continueButton = new QPushButton("Продолжить", this);
    m_continueButton->setMinimumHeight(50);

    m_continueButton->setEnabled(false);
    m_continueButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color:  white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover: enabled {
            background-color: #43a047;
        }
        QPushButton:disabled {
            background-color: #ccc;
        }
    )");

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(selectorLayout);
    mainLayout->addLayout(legendLayout);
    mainLayout->addWidget(m_selectedSeatLabel);
    mainLayout->addWidget(m_scrollArea, 1);

    QHBoxLayout* buttonBar = new QHBoxLayout();
    buttonBar->addStretch();
    buttonBar->addWidget(m_continueButton);
    buttonBar->addStretch();
    mainLayout->addLayout(buttonBar);


    connect(m_backButton, &QPushButton::clicked, this, &SeatSelectionWidget::onBackClicked);
    connect(m_continueButton, &QPushButton::clicked, this, &SeatSelectionWidget::onContinueClicked);
    connect(m_carriageSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SeatSelectionWidget::onCarriageChanged);
}

void SeatSelectionWidget::loadSeats(const TrainSearchResult& train, int depId, int arrId)
{
    m_currentTrain = train;
    m_departureStationId = depId;
    m_arrivalStationId = arrId;
    m_selectedSeat = Seat();
    m_selectedButton = nullptr;

    m_trainInfoLabel->setText(QString("Поезд №%1 - Выбор места").arg(train.trainNumber));
    m_selectedSeatLabel->setText("Выберите место");
    m_continueButton->setEnabled(false);
    m_carriageSelector->clear();

    for (auto* w: m_carriageWidgets) {
        delete w;
    }
    m_carriageWidgets.clear();

    ApiClient::instance().getAvailableSeats(train.scheduleId, depId, arrId);
}

void SeatSelectionWidget::onSeatsReceived(QList<Seat> seats){
    m_seats = seats;
    createCarriageWidgets();
}

void SeatSelectionWidget::setBookedSeats(const QList<int>& bookedSeatIds)
{
    m_bookedSeatIds = bookedSeatIds;
    for (CarriageWidget* cw: m_carriageWidgets) {
        cw->markSeatsAsBooked(bookedSeatIds);
    }
}

void SeatSelectionWidget::createCarriageWidgets()
{
    QLayoutItem* item;
    while ((item = m_carriagesLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_carriageWidgets.clear();
    m_carriageSelector->clear();

    if (m_seats.isEmpty()) {
        QLabel* noSeatsLabel = new QLabel("Свободные места отсутствуют", m_scrollContent);
        noSeatsLabel->setAlignment(Qt::AlignCenter);
        noSeatsLabel->setStyleSheet("font-size: 16px; color: #999; padding: 50px;");
        m_carriagesLayout->addWidget(noSeatsLabel);
        return;
    }

    QMap<int, QList<Seat>> carriageSeats;
    QMap<int, QString> carriageTypes;

    for (const Seat& seat : m_seats) {
        int cnum = seat.carriageNumber;
        if (cnum == 0) {
            qWarning() << "Seat has no carriageNumber, fallback to carriageId:" << seat.id << seat.carriageId;
            cnum = seat.carriageId;
        }
        carriageSeats[cnum].append(seat);
        carriageTypes[cnum] = seat.carriageType;
    }

    QList<int> carriageNumbers = carriageSeats.keys();
    std::sort(carriageNumbers.begin(), carriageNumbers.end());

    for (int carriageNumber: carriageNumbers) {
        QList<Seat> seats = carriageSeats[carriageNumber];
        std::sort(seats.begin(), seats.end(), [](const Seat& a, const Seat& b) {
            return a.seatNumber < b.seatNumber;
        });

        QString carriageType = carriageTypes.value(carriageNumber, QString("Вагон"));
        CarriageWidget* cw = new CarriageWidget(carriageNumber, carriageType, seats, m_scrollContent);
        connect(cw, &CarriageWidget::seatClicked, this, &SeatSelectionWidget::onSeatClicked);

        if (!m_bookedSeatIds.isEmpty()) {
            cw->markSeatsAsBooked(m_bookedSeatIds);
        }

        m_carriageWidgets.append(cw);
        m_carriagesLayout->addWidget(cw);

        m_carriageSelector->addItem(
            QString("Вагон %1 (%2) - %3 мест").arg(carriageNumber).arg(carriageType).arg(seats.size()),
            carriageNumber
            );
    }

    m_carriagesLayout->addStretch();
}

void SeatSelectionWidget::onSeatClicked(const Seat& seat, SeatWidget* button)
{
    if (m_selectedButton == button && button->isChecked()) {
        button->setSelected(false);
        m_selectedButton = nullptr;
        m_selectedSeat = Seat();

        m_selectedSeatLabel->setText("Выберите место");
        m_selectedSeatLabel->setStyleSheet(R"(
            QLabel {
                background-color: #f5f5f5;
                border: 1px solid #e0e0e0;
                border-radius: 5px;
                padding: 12px;
                font-size: 14px;
                color: #666;
            }
        )");
        m_continueButton->setEnabled(false);
        return;
    }

    for (CarriageWidget* cw: qAsConst(m_carriageWidgets)) {
        cw->clearSelection();
    }

    if (button) {
        button->setSelected(true);
    }
    m_selectedButton = button;
    m_selectedSeat = seat;

    m_selectedSeatLabel->setText(
        QString("✓ Выбрано: Место №%1 (%2)").arg(seat.seatNumber).arg(seat.seatType)
        );
    m_selectedSeatLabel->setStyleSheet(R"(
        QLabel {
            background-color: #e8f5e9;
            border: 2px solid #4CAF50;
            border-radius: 5px;
            padding: 12px;
            font-size: 14px;
            font-weight: bold;
            color: #2e7d32;
        }
    )");
    m_continueButton->setEnabled(true);
}

void SeatSelectionWidget::displaySeats()
{
    createCarriageWidgets();
}

void SeatSelectionWidget::onCarriageChanged(int index){
    if (index < 0 || index >= m_carriageWidgets.size()) return;

    CarriageWidget* cw = m_carriageWidgets[index];
    m_scrollArea->ensureWidgetVisible(cw);
}

void SeatSelectionWidget::onBackClicked()
{
    emit backRequested();
}

void SeatSelectionWidget::onContinueClicked()
{
    if (m_selectedSeat.id <= 0) {
        QMessageBox::warning(this, "Ошибка", "Пожалуйста, выберите место");
        return;
    }

    emit seatSelected(m_currentTrain, m_selectedSeat,
                      m_departureStationId, m_arrivalStationId,
                      m_currentTrain.minPrice);
}
