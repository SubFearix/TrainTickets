#ifndef CARRIAGEWIDGET_H
#define CARRIAGEWIDGET_H

#include <QWidget>
#include <QMap>
#include <QPushButton>
#include <QScrollArea>
#include "apiclient.h"

class SeatWidget : public QPushButton
{
    Q_OBJECT

public:
    explicit SeatWidget(const Seat& seat, QWidget* parent = nullptr);
    Seat getSeat() const { return m_seat; }
    void setOccupied(bool occupied);
    void setBooked(bool booked);
    void setSelected(bool selected);
    void updateStyle();
    bool isOccupied() const { return m_occupied; }
    bool isBooked() const { return m_booked; }

private:
    Seat m_seat;
    bool m_occupied;
    bool m_booked;
};

class CarriageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CarriageWidget(int carriageNumber
                            , const QString& carriageType
                            , const QList<Seat>& seats
                            , QWidget *parent = nullptr);
    void clearSelection();
    void markSeatsAsBooked(const QList<int>& seatNumbers);
    void paintEvent(QPaintEvent* event) override;

signals:
    void seatClicked(const Seat& seat,  SeatWidget* button);

private:
    void setupPlatzkartLayout(const QList<Seat>& seats);
    void setupKupeLayout(const QList<Seat>& seats);
    void setupSidyachiyLayout(const QList<Seat>& seats);
    void arrangeSidyachiyLayout();

    int m_carriageNumber;
    QString m_carriageType;
    QList<Seat> m_seatsData;
    QMap<int, SeatWidget*> m_seatWidgets;
};

#endif // CARRIAGEWIDGET_H
