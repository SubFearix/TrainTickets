#ifndef MAINMENUWIDGET_H
#define MAINMENUWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QListWidget>
#include "apiclient.h"

class MainMenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenuWidget(QWidget *parent = nullptr);
    void initialize();

signals:
    void searchRequested(int depId, QString depName, int arrId, QString arrName, QDate date);
    void myTicketsRequested();
    void profileRequested();
    void logoutRequested();

private slots:
    void onSearchClicked();
    void onStationsReceived(QList<Station> stations);
    void onPopularRouteClicked(QListWidgetItem* item);
    void onMyTicketsClicked();
    void onProfileClicked();
    void onLogoutClicked();

private:
    void setupUi();
    void loadStations();
    void setupPopularRoutes();

    QList<Station> m_stations;

    QLineEdit* m_departureInput;
    QLineEdit* m_arrivalInput;

    QCompleter* m_departureCompleter;
    QCompleter* m_arrivalCompleter;

    QMap<QString, int> m_departureStationMap;
    QMap<QString, int> m_arrivalStationMap;
    QDateEdit* m_dateEdit;
    QPushButton* m_searchButton;
    QListWidget* m_popularRoutesList;
    QPushButton* m_myTicketsButton;
    QPushButton* m_profileButton;
    QPushButton* m_logoutButton;
};

#endif // MAINMENUWIDGET_H
