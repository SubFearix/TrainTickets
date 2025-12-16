#ifndef BOOKINGWIDGET_H
#define BOOKINGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QTimer>
#include "apiclient.h"

class BookingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookingWidget(QWidget *parent = nullptr);
    void setBookingData(const TrainSearchResult& train
                        , const Seat& seat
                        , int depId
                        , int arrId
                        , double price);

signals:
    void backRequested();
    void bookingCompleted();

private slots:
    void onUseMyselfClicked(bool checked);
    void onBookClicked();
    void onPayClicked();
    void onCancelBookingClicked();
    void onTicketBooked(QString ticketNumber, QString status);
    void onTicketPaid(QString ticketNumber);
    void onError(QString errorMessage);
    void updateTimer();

private:
    void setupUi();
    void showBookingForm();
    void showPaymentForm();
    void resetForm();
    bool validateForm();

    QLabel* m_tripInfoLabel;
    QLabel* m_seatInfoLabel;
    QLabel* m_priceLabel;

    QLineEdit* m_passengerNameEdit;
    QLineEdit* m_passengerDocumentEdit;
    QCheckBox* m_useMyselfCheckbox;

    QPushButton* m_bookButton;
    QPushButton* m_payButton;
    QPushButton* m_cancelBookingButton;
    QPushButton* m_backButton;

    QLabel* m_statusLabel;
    QLabel* m_timerLabel;
    QLabel* m_errorLabel;

    QWidget* m_bookingFormWidget;
    QWidget* m_paymentFormWidget;

    TrainSearchResult m_train;
    Seat m_seat;
    int m_departureStationId;
    int m_arrivalStationId;
    double m_price;
    QString m_currentTicketNumber;

    QTimer* m_bookingTimer;
    QDateTime m_bookingTime;
    int m_timeoutMinutes;
};

#endif // BOOKINGWIDGET_H
