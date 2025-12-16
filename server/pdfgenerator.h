#ifndef PDFGENERATOR_H
#define PDFGENERATOR_H

#include <QString>
#include <QByteArray>
#include "database.h"

class PdfGenerator
{
public:
    PdfGenerator();

    static QByteArray generateTicketPdf(const Ticket& ticket
                                        , const QString& trainNumber
                                        , const QString& trainType
                                        , const QString& departureStationName
                                        , const QString& arrivalStationName
                                        , const QDateTime& departureTime
                                        , const QDateTime& arrivalTime
                                        , int carriageNumber
                                        , int seatNumber);

    static bool saveToFile(const QByteArray& pdfData, const QString& filename);

private:
    static QString generateHtmlTicket(const Ticket& ticket
                                      , const QString& trainNumber
                                      , const QString& trainType
                                      , const QString& departureStationName
                                      , const QString& arrivalStationName
                                      , const QDateTime& departureTime
                                      , const QDateTime& arrivalTime
                                      , int carriageNumber
                                      , int seatNumber);
};

#endif // PDFGENERATOR_H
