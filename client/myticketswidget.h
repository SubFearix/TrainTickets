#ifndef MYTICKETSWIDGET_H
#define MYTICKETSWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include "apiclient.h"

class MyTicketsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyTicketsWidget(QWidget *parent = nullptr);

    void loadTickets();

signals:
    void backRequested();

private slots:
    void onTicketsReceived(QList<Ticket> tickets);
    void onTicketItemClicked(QListWidgetItem* item);
    void onBackClicked();
    void onRefreshClicked();
    void onCancelTicketClicked();

private:
    void setupUi();
    void displayTickets();
    QString getStatusText(const QString& status) const;
    QString getStatusColor(const QString& status) const;

    QLabel* m_titleLabel;
    QTabWidget* m_tabWidget;
    QListWidget* m_activeTicketsList;
    QListWidget* m_historyTicketsList;
    QPushButton* m_backButton;
    QPushButton* m_refreshButton;

    QList<Ticket> m_tickets;
    Ticket m_selectedTicket;
};

#endif // MYTICKETSWIDGET_H
