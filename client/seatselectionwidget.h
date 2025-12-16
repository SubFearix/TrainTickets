#ifndef SEATSELECTIONWIDGET_H
#define SEATSELECTIONWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QScrollArea>
#include "apiclient.h"
#include "carriagewidget.h"

class SeatSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SeatSelectionWidget(QWidget *parent = nullptr);

    void loadSeats(const TrainSearchResult& train, int depId, int arrId);
    void setBookedSeats(const QList<int>& bookedSeatIds);

signals:
    void backRequested();
    void seatSelected(TrainSearchResult train, Seat seat, int depId, int arrId, double price);

private slots:
    void onSeatsReceived(QList<Seat> seats);
    void onSeatClicked(const Seat& seat, SeatWidget* button);
    void onCarriageChanged(int index);
    void onBackClicked();
    void onContinueClicked();

private:
    void setupUi();
    void displaySeats();
    void createCarriageWidgets();

    QLabel* m_trainInfoLabel;
    QLabel* m_selectedSeatLabel;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QVBoxLayout* m_carriagesLayout;
    QPushButton* m_backButton;
    QPushButton* m_continueButton;
    QComboBox* m_carriageSelector;

    TrainSearchResult m_currentTrain;
    int m_departureStationId;
    int m_arrivalStationId;
    QList<Seat> m_seats;
    Seat m_selectedSeat;
    SeatWidget* m_selectedButton;
    QList<CarriageWidget*> m_carriageWidgets;
    QList<int> m_bookedSeatIds;
};

#endif // SEATSELECTIONWIDGET_H
