#include "trainlistwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

TrainListWidget::TrainListWidget(QWidget *parent)
    : QWidget{parent}
{
    setupUi();

    connect(&ApiClient::instance(), &ApiClient::trainsReceived,
            this, &TrainListWidget::onTrainsReceived);
}

void TrainListWidget::setupUi(){
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

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

    QVBoxLayout* infoLayout = new QVBoxLayout();
    m_routeLabel = new QLabel(this);
    QFont routeFont = m_routeLabel->font();
    routeFont.setPointSize(16);
    routeFont.setBold(true);
    m_routeLabel->setFont(routeFont);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setStyleSheet("color: #666;");

    infoLayout->addWidget(m_routeLabel);
    infoLayout->addWidget(m_dateLabel);

    headerLayout->addWidget(m_backButton);
    headerLayout->addLayout(infoLayout);
    headerLayout->addStretch();

    m_trainsList = new QListWidget(this);
    m_trainsList->setStyleSheet(R"(
        QListWidget {
            border: 1px solid #ddd;
            border-radius: 5px;
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

    m_noResultsLabel = new QLabel("Поезда не найдены", this);
    m_noResultsLabel->setAlignment(Qt::AlignCenter);
    QFont noResultsFont = m_noResultsLabel->font();
    noResultsFont.setPointSize(14);
    m_noResultsLabel->setFont(noResultsFont);
    m_noResultsLabel->setStyleSheet("color: #999;");
    m_noResultsLabel->hide();

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_trainsList);
    mainLayout->addWidget(m_noResultsLabel);

    connect(m_backButton, &QPushButton::clicked, this, &TrainListWidget::onBackClicked);
    connect(m_trainsList, &QListWidget::itemClicked, this, &TrainListWidget::onTrainItemClicked);
}

void TrainListWidget::searchTrains(int depId, QString depName, int arrId, QString arrName, QDate date){
    m_departureStationId = depId;
    m_arrivalStationId = arrId;
    m_departureName = depName;
    m_arrivalName = arrName;
    m_searchDate = date;

    m_routeLabel->setText(QString("%1 → %2").arg(depName, arrName));
    m_dateLabel->setText(date.toString("dd MMMM yyyy"));

    m_trainsList->clear();
    m_noResultsLabel->hide();

    QListWidgetItem* loadingItem = new QListWidgetItem("Поиск поездов...");
    loadingItem->setTextAlignment(Qt::AlignCenter);
    loadingItem->setFlags(Qt::NoItemFlags);
    m_trainsList->addItem(loadingItem);

    ApiClient::instance().searchTrains(depId, arrId, date);
}

void TrainListWidget::onTrainsReceived(QList<TrainSearchResult> trains){
    m_trains = trains;
    m_trainsList->clear();

    if (trains.isEmpty()) {
        m_noResultsLabel->show();
        return;
    }

    m_noResultsLabel->hide();

    for (const TrainSearchResult& train: trains) {
        QWidget* itemWidget = new QWidget();
        QVBoxLayout* itemLayout = new QVBoxLayout(itemWidget);
        itemLayout->setContentsMargins(10, 10, 10, 10);
        itemLayout->setSpacing(5);

        QHBoxLayout* topLine = new QHBoxLayout();
        QLabel* trainInfo = new QLabel(QString("Поезд №%1 (%2)")
                                           .arg(train.trainNumber, train.trainType));
        QFont trainFont = trainInfo->font();
        trainFont.setBold(true);
        trainFont.setPointSize(12);
        trainInfo->setFont(trainFont);

        QLabel* seatsLabel = new QLabel(QString("Мест: %1").arg(train.availableSeats));
        seatsLabel->setStyleSheet("color: #4CAF50;");

        topLine->addWidget(trainInfo);
        topLine->addStretch();
        topLine->addWidget(seatsLabel);

        QHBoxLayout* timeLine = new QHBoxLayout();
        QLabel* depTime = new QLabel(train.departureTime.toString("HH:mm"));
        QFont timeFont = depTime->font();
        timeFont.setPointSize(16);
        depTime->setFont(timeFont);

        QLabel* arrow = new QLabel("→");
        arrow->setAlignment(Qt::AlignCenter);

        QLabel* arrTime = new QLabel(train.arrivalTime.toString("HH:mm"));
        arrTime->setFont(timeFont);

        QLabel* duration = new QLabel(formatDuration(train.travelTimeMinutes));
        duration->setStyleSheet("color: #666;");
        duration->setAlignment(Qt::AlignCenter);

        timeLine->addWidget(depTime);
        timeLine->addWidget(arrow);
        timeLine->addWidget(arrTime);
        timeLine->addSpacing(20);
        timeLine->addWidget(duration);
        timeLine->addStretch();

        QHBoxLayout* priceLine = new QHBoxLayout();
        QLabel* priceLabel = new QLabel(QString("от %1 ₽").arg(train.minPrice, 0, 'f', 2));
        QFont priceFont = priceLabel->font();
        priceFont.setPointSize(14);
        priceFont.setBold(true);
        priceLabel->setFont(priceFont);
        priceLabel->setStyleSheet("color: #2196F3;");

        priceLine->addStretch();
        priceLine->addWidget(priceLabel);

        itemLayout->addLayout(topLine);
        itemLayout->addLayout(timeLine);
        itemLayout->addLayout(priceLine);

        QFrame* separator = new QFrame();
        separator->setFrameShape(QFrame::HLine);
        separator->setStyleSheet("background-color: #eee;");

        QListWidgetItem* item = new QListWidgetItem(m_trainsList);
        item->setSizeHint(itemWidget->sizeHint() + QSize(0, 20));
        m_trainsList->addItem(item);
        m_trainsList->setItemWidget(item, itemWidget);
    }
}

void TrainListWidget::onTrainItemClicked(QListWidgetItem *item){
    int index = m_trainsList->row(item);
    if (index >= 0 && index < m_trains.size()) {
        emit trainSelected(m_trains[index], m_departureStationId, m_arrivalStationId);
    }
}

void TrainListWidget::onBackClicked(){
    emit backRequested();
}

QString TrainListWidget::formatDuration(int minutes) const{
    int hours = minutes / 60;
    int mins = minutes % 60;

    if (hours > 0) {
        return QString("%1 ч %2 мин").arg(hours).arg(mins);
    } else {
        return QString("%1 мин").arg(mins);
    }
}
