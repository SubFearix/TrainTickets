#ifndef TRAINLISTWIDGET_H
#define TRAINLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include "apiclient.h"

class TrainListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrainListWidget(QWidget *parent = nullptr);

    void searchTrains(int depId, QString depName, int arrId, QString arrName, QDate date);

signals:
    void backRequested();
    void trainSelected(TrainSearchResult train, int depId, int arrId);

private slots:
    void onTrainsReceived(QList<TrainSearchResult> trains);
    void onTrainItemClicked(QListWidgetItem* item);
    void onBackClicked();

private:
    void setupUi();
    QString formatDuration(int minutes) const;

    QLabel* m_routeLabel;
    QLabel* m_dateLabel;
    QListWidget* m_trainsList;
    QPushButton* m_backButton;
    QLabel* m_noResultsLabel;

    int m_departureStationId;
    int m_arrivalStationId;
    QString m_departureName;
    QString m_arrivalName;
    QDate m_searchDate;

    QList<TrainSearchResult> m_trains;
};

#endif // TRAINLISTWIDGET_H
