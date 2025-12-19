#include "carriagewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>

SeatWidget::SeatWidget(const Seat& seat, QWidget* parent)
    : QPushButton(QString::number(seat.seatNumber), parent)
    , m_seat(seat)
    , m_occupied(false)
    , m_booked(false)
{
    setCheckable(true);
    setFixedSize(32, 28);
    setFont(QFont("Arial", 9, QFont::Bold));
    setCursor(Qt::PointingHandCursor);
    updateStyle();
}

void SeatWidget::setOccupied(bool occupied){
    m_occupied = occupied;
    setEnabled(!occupied && !m_booked);
    updateStyle();
}

void SeatWidget::setBooked(bool booked){
    m_booked = booked;
    setEnabled(!booked && !m_occupied);
    updateStyle();
}

void SeatWidget::setSelected(bool selected){
    setChecked(selected);
    updateStyle();
}

void SeatWidget::updateStyle(){
    QString bgColor, textColor, borderColor;

    if (m_booked) {
        bgColor = "#ef5350";
        textColor = "white";
        borderColor = "#c62828";
    } else if (m_occupied) {
        bgColor = "#ffcdd2";
        textColor = "#c62828";
        borderColor = "#ef9a9a";
    } else if (isChecked()) {
        bgColor = "#f44336";
        textColor = "white";
        borderColor = "#c62828";
    } else {
        QString type = m_seat.seatType.toLower();
        int num = m_seat.seatNumber;

        bool isSide = (num >= 37 && num <= 54);

        if (type.contains("сидяч")) {
            bgColor = "#e0f7fa";
            textColor = "#00838f";
            borderColor = "#80deea";
        } else if (isSide && type.contains("нижн")) {
            bgColor = "#e8f5e9";
            textColor = "#2e7d32";
            borderColor = "#a5d6a7";
        } else if (isSide && type.contains("верхн")) {
            bgColor = "#fff3e0";
            textColor = "#e65100";
            borderColor = "#ffcc80";
        } else if (type.contains("нижн")) {
            bgColor = "#e3f2fd";
            textColor = "#1565c0";
            borderColor = "#90caf9";
        } else if (type.contains("верхн")) {
            bgColor = "#f3e5f5";
            textColor = "#7b1fa2";
            borderColor = "#ce93d8";
        } else {
            bgColor = "#f5f5f5";
            textColor = "#424242";
            borderColor = "#bdbdbd";
        }
    }

    setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 2px solid %3;
            border-radius: 4px;
            font-weight: bold;
            font-size: 10px;
        }
        QPushButton:hover:enabled {
            background-color: %3;
            color: white;
        }
        QPushButton:checked {
            background-color: #f44336;
            color: white;
            border-color: #c62828;
        }
        QPushButton:disabled {
            opacity: 0.7;
        }
    )").arg(bgColor, textColor, borderColor));
}

CarriageWidget::CarriageWidget(int carriageNumber
                               , const QString &carriageType
                               , const QList<Seat> &seats
                               , QWidget *parent)
    : QWidget{parent}
    , m_carriageNumber(carriageNumber)
    , m_carriageType(carriageType)
{
    m_seatsData = seats;
    setMinimumHeight(200);

    if (m_carriageType == "Плацкарт") {
        setupPlatzkartLayout(m_seatsData);
        setMinimumWidth(900);
        setFixedHeight(180);
    } else if (m_carriageType == "Купе") {
        setupKupeLayout(m_seatsData);
        setMinimumWidth(700);
        setFixedHeight(160);
    } else {
        setupSidyachiyLayout(m_seatsData);
    }
}

void CarriageWidget::clearSelection()
{
    for (auto* seat: m_seatWidgets) {
        seat->setChecked(false);
        seat->updateStyle();
    }
}

void CarriageWidget::markSeatsAsBooked(const QList<int>& seatNumbers)
{
    for (int seatNum : seatNumbers) {
        if (m_seatWidgets.contains(seatNum)) {
            m_seatWidgets[seatNum]->setBooked(true);
        }
    }
}

void CarriageWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter:: Antialiasing);

    QPainterPath path;
    int margin = 10;
    int w = width() - 2 * margin;
    int h = height() - 2 * margin;
    int radius = 15;

    path.addRoundedRect(margin, margin, w, h, radius, radius);

    painter.fillPath(path, QColor("#fafafa"));
    painter.setPen(QPen(QColor("#bdbdbd"), 2));
    painter.drawPath(path);

    int centerY = height() / 2;
    painter.setPen(QPen(QColor("#e0e0e0"), 1, Qt::DashLine));
    painter.drawLine(margin + 50, centerY, width() - margin - 50, centerY);

    painter. setPen(QColor("#424242"));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(15, 25, QString("Вагон %1").arg(m_carriageNumber));

    painter.setFont(QFont("Arial", 9));
    painter.setPen(QColor("#757575"));
    painter.drawText(15, 40, m_carriageType);

    painter.setPen(QColor("#9e9e9e"));
    painter.setFont(QFont("Arial", 8));
    painter.drawText(margin + 15, height() - margin - 15, "WC");

    painter.drawText(width() - margin - 30, margin + 25, "WC");
}

void CarriageWidget::setupPlatzkartLayout(const QList<Seat>& seats)
{
    int startX = 140;
    int upperY = 35;
    int lowerY = 65;
    int sideUpperY = 110;
    int sideLowerY = 140;
    int seatW = 36;
    int seatH = 26;
    int pairGap = 4;
    int compartmentGap = 12;

    for (const Seat& seat: seats) {
        SeatWidget* sw = new SeatWidget(seat, this);
        m_seatWidgets[seat.seatNumber] = sw;

        sw->setOccupied(!seat.isAvailable);

        connect(sw, &QPushButton::clicked, this, [this, sw]() {
            sw->setChecked(!sw->isChecked());
            emit seatClicked(sw->getSeat(), sw);
        });

        int num = seat.seatNumber;
        int x, y;

        if (num >= 1 && num <= 36) {
            int compartment = (num - 1) / 4;
            int posInPair = ((num - 1) / 2) % 2;

            x = startX + compartment * (2 * seatW + pairGap + compartmentGap)
                + posInPair * (seatW + pairGap);

            if (num % 2 == 1) {
                y = lowerY;
            } else {
                y = upperY;
            }
        } else if (num >= 37 && num <= 54) {
            int sideCompartment = (num - 37) / 2;

            x = startX + sideCompartment * (seatW + compartmentGap + pairGap);
            if (num % 2 == 1) {
                y = sideLowerY;
            } else {
                y = sideUpperY;
            }
        } else {
            continue;
        }

        sw->setGeometry(x, y, seatW, seatH);
    }
}

void CarriageWidget::setupKupeLayout(const QList<Seat>& seats)
{
    int startX = 140;
    int topY = 40;
    int bottomY = 75;
    int seatW = 36;
    int seatH = 30;
    int gapX = 6;
    int compartmentGap = 25;

    for (const Seat& seat: seats) {
        SeatWidget* sw = new SeatWidget(seat, this);
        m_seatWidgets[seat.seatNumber] = sw;

        sw->setOccupied(!seat.isAvailable);

        connect(sw, &QPushButton::clicked, this, [this, sw]() {
            sw->setChecked(!sw->isChecked());
            emit seatClicked(sw->getSeat(), sw);
        });

        int num = seat.seatNumber;
        int compartment = (num - 1) / 4;
        int posInPair = ((num - 1) / 2) % 2;

        int x = startX + compartment * (2 * seatW + gapX + compartmentGap)
                + posInPair * (seatW + gapX);
        int y = (num % 2 == 1) ? bottomY : topY;

        sw->setGeometry(x, y, seatW, seatH);
    }
}

void CarriageWidget::setupSidyachiyLayout(const QList<Seat>& seats)
{
    for (auto w : m_seatWidgets) delete w;
    m_seatWidgets.clear();

    m_seatsData = seats;
    for (const Seat& seat: qAsConst(m_seatsData)) {
        SeatWidget* sw = new SeatWidget(seat, this);
        m_seatWidgets[seat.seatNumber] = sw;

        sw->setOccupied(!seat.isAvailable);

        connect(sw, &QPushButton::clicked, this, [this, sw]() {
            sw->setChecked(!sw->isChecked());
            emit seatClicked(sw->getSeat(), sw);
        });
        sw->setGeometry(0,0,36,28);
        sw->show();
    }

    QTimer::singleShot(0, this, &CarriageWidget::arrangeSidyachiyLayout);
}

void CarriageWidget::arrangeSidyachiyLayout()
{
    const int seatW = 36;
    const int seatH = 28;
    const int innerGapX = 4;
    const int innerGapY = 8;
    const int aisleHeight = 20;
    const int blockHgap = 16;
    const int margin = 16;
    const int startX = 140;

    int count = m_seatsData.size();
    if (count == 0) {
        setMinimumHeight(80);
        return;
    }

    const int seatsPerBlock = 4;
    int blocksNeeded = (count + seatsPerBlock - 1) / seatsPerBlock;

    int topY = 40;
    int bottomY = topY + seatH + innerGapY + aisleHeight;

    for (int bi = 0; bi < blocksNeeded; ++bi) {
        int blockWidth = 2 * seatW + innerGapX;
        int blockOriginX = startX + bi * (blockWidth + blockHgap);

        for (int pos = 0; pos < seatsPerBlock; ++pos) {
            int globalIndex = bi * seatsPerBlock + pos;
            if (globalIndex >= count) break;

            const Seat& seat = m_seatsData[globalIndex];
            SeatWidget* sw = m_seatWidgets.value(seat.seatNumber, nullptr);
            if (!sw) continue;

            int x, y;

            if (pos == 0) {
                x = blockOriginX;
                y = topY;
            } else if (pos == 1) {
                x = blockOriginX + seatW + innerGapX;
                y = topY;
            } else if (pos == 2) {
                x = blockOriginX;
                y = bottomY;
            } else {
                x = blockOriginX + seatW + innerGapX;
                y = bottomY;
            }

            sw->setGeometry(x, y, seatW, seatH);
            sw->show();
        }
    }

    int blockWidth = 2 * seatW + innerGapX;
    int neededWidth = startX + blocksNeeded * blockWidth + (blocksNeeded - 1) * blockHgap + margin;
    int neededHeight = bottomY + seatH + margin;

    setMinimumWidth(neededWidth);
    setMinimumHeight(neededHeight);
    setFixedHeight(neededHeight);
    updateGeometry();
    update();
}
