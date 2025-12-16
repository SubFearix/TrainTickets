#include "pdfgenerator.h"
#include <QPdfWriter>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QBuffer>
#include <QFile>
#include <QTextDocument>

PdfGenerator::PdfGenerator() {}

QByteArray PdfGenerator::generateTicketPdf(const Ticket &ticket
                                           , const QString &trainNumber
                                           , const QString &trainType
                                           , const QString &departureStationName
                                           , const QString &arrivalStationName
                                           , const QDateTime &departureTime
                                           , const QDateTime &arrivalTime
                                           , int carriageNumber
                                           , int seatNumber)
{
    QString html = generateHtmlTicket(ticket
                                      , trainNumber
                                      , trainType
                                      , departureStationName
                                      , arrivalStationName
                                      , departureTime
                                      , arrivalTime
                                      , carriageNumber
                                      , seatNumber);
    QByteArray pdfData;
    QBuffer buffer(&pdfData);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);

    writer.setPageSize(QPageSize::A4);
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    writer.setResolution(300);

    QTextDocument document;
    document.setHtml(html);
    document.print(&writer);

    buffer.close();

    qDebug() << "PDF generated, size:" << pdfData.size() << "bytes";

    return pdfData;
}

bool PdfGenerator::saveToFile(const QByteArray& pdfData, const QString& filename){
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open file for writing:" << filename;
        return false;
    }

    file.write(pdfData);
    file.close();

    qDebug() << "PDF saved to:" << filename;
    return true;
}


QString PdfGenerator::generateHtmlTicket(const Ticket& ticket
                                         , const QString& trainNumber
                                         , const QString& trainType
                                         , const QString& departureStationName
                                         , const QString& arrivalStationName
                                         , const QDateTime& departureTime
                                         , const QDateTime& arrivalTime
                                         , int carriageNumber
                                         , int seatNumber)
{
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: Arial, sans-serif;
            font-size: 16px;
            margin: 20px;
        }
        .ticket-container {
            border: 3px solid #2196F3;
            border-radius: 10px;
            padding: 30px;
            background: linear-gradient(to bottom, #ffffff 0%, #f5f5f5 100%);
            width: 100%;
            max-width: 600px;
            margin: 0 auto;
        }
        .header {
            text-align: center;
            border-bottom: 2px solid #2196F3;
            padding-bottom: 20px;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #2196F3;
            margin: 0;
            font-size: 32px;
        }
        .header .subtitle {
            color: #666;
            margin-top: 5px;
            font-size: 14px;
        }
        .ticket-number {
            background: #2196F3;
            color: white;
            padding: 10px 20px;
            border-radius: 5px;
            display: inline-block;
            font-size: 28px;
            font-weight: bold;
            margin: 20px 0;
        }
        .info-section {
            margin: 30px 0;
        }
        .info-row {
            display: table;
            width: 100%;
            margin: 15px 0;
            border-bottom: 1px solid #ddd;
            padding-bottom: 10px;
        }
        .info-label {
            display: table-cell;
            width: 40%;
            font-weight: bold;
            color: #555;
            font-size: 18px;
        }
        .info-value {
            display: table-cell;
            width: 60%;
            color: #000;
            font-size: 20px;
        }
        .route-section {
            background: #E3F2FD;
            padding: 20px;
            border-radius: 8px;
            margin: 30px 0;
        }
        .route-info {
            text-align: center;
            font-size: 32px;
            font-weight: bold;
            color: #1976D2;
            margin: 20px 0;
        }
        .time-section {
            display: table;
            width: 100%;
            margin: 20px 0;
        }
        .time-block {
            display: table-cell;
            width: 40%;
            text-align: center;
        }
        .arrow {
            display: table-cell;
            width: 20%;
            text-align: center;
            font-size: 30px;
            color: #2196F3;
        }
        .time {
            font-size: 36px;
            font-weight: bold;
            color: #2196F3;
        }
        .date {
            font-size: 18px;
            color: #666;
            margin-top: 5px;
        }
        .passenger-section {
            background: #FFF3E0;
            padding: 20px;
            border-radius: 8px;
            margin: 30px 0;
        }
        .status-paid {
            background: #4CAF50;
            color: white;
            padding: 15px;
            border-radius: 5px;
            text-align: center;
            font-size: 18px;
            font-weight: bold;
            margin: 30px 0;
        }
        .footer {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 2px solid #ddd;
            text-align: center;
            color: #666;
            font-size: 16px;
        }
        .qr-placeholder {
            width: 150px;
            height: 150px;
            border: 2px dashed #999;
            margin: 20px auto;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #999;
        }

        /* Print-specific styles for PDF generation */
        @media print {
            @page {
                size: A4;
                margin: 20mm;
            }
            body {
                margin: 0;
                -webkit-print-color-adjust: exact;
                color-adjust: exact;
                font-size: 16px;
            }
            .ticket-container {
                border: 2px solid #2196F3; /* Thinner border for print */
                padding: 20px; /* Reduced padding */
                background: white; /* Solid background for print */
                width: auto;
                max-width: none;
                margin: 0;
                page-break-inside: avoid;
            }
            .header h1 {
                font-size: 32px; /* Slightly smaller for print */
            }
            .route-info {
                font-size: 28px; /* Adjusted for print */
            }
            .time {
                font-size: 32px; /* Adjusted for print */
            }
            .ticket-number {
                font-size: 24px;
            }
            .info-label {
                font-size: 16px;
            }
            .info-value {
                font-size: 18px;
            }
            .date {
                font-size: 16px;
            }
            .footer {
                font-size: 14px;
            }
            .qr-placeholder {
                border: 1px solid #999; /* Thinner border */
            }
            /* Ensure no page breaks inside sections */
            .info-section, .route-section, .passenger-section {
                page-break-inside: avoid;
            }
        }
    </style>
</head>
<body>
    <div class="ticket-container">
        <div class="header">
            <h1>🚂 ЖЕЛЕЗНОДОРОЖНЫЙ БИЛЕТ</h1>
            <div class="subtitle">Система бронирования билетов</div>
        </div>

        <div style="text-align: center;">
            <div class="ticket-number">Билет №%1</div>
        </div>

        <div class="status-paid">
            ✓ ОПЛАЧЕН
        </div>

        <div class="route-section">
            <div class="route-info">%2 → %3</div>

            <div class="time-section">
                <div class="time-block">
                    <div class="time">%4</div>
                    <div class="date">%5</div>
                    <div style="margin-top: 10px; color: #666;">Отправление</div>
                </div>

                <div class="arrow">→</div>

                <div class="time-block">
                    <div class="time">%6</div>
                    <div class="date">%7</div>
                    <div style="margin-top: 10px; color: #666;">Прибытие</div>
                </div>
            </div>
        </div>

        <div class="info-section">
            <h2 style="color: #2196F3; border-bottom: 2px solid #2196F3; padding-bottom: 10px;">Информация о поезде</h2>

            <div class="info-row">
                <div class="info-label">Номер поезда:</div>
                <div class="info-value">%8</div>
            </div>

            <div class="info-row">
                <div class="info-label">Тип поезда:</div>
                <div class="info-value">%9</div>
            </div>

            <div class="info-row">
                <div class="info-label">Вагон:</div>
                <div class="info-value">№%10</div>
            </div>

            <div class="info-row">
                <div class="info-label">Место:</div>
                <div class="info-value">№%11</div>
            </div>
        </div>

        <div class="passenger-section">
            <h2 style="color: #F57C00; margin-top: 0;">Информация о пассажире</h2>

            <div class="info-row">
                <div class="info-label">ФИО:</div>
                <div class="info-value">%12</div>
            </div>

            <div class="info-row">
                <div class="info-label">Документ:</div>
                <div class="info-value">%13</div>
            </div>
        </div>

        <div class="info-section">
            <h2 style="color: #2196F3; border-bottom: 2px solid #2196F3; padding-bottom: 10px;">Информация об оплате</h2>

            <div class="info-row">
                <div class="info-label">Стоимость:</div>
                <div class="info-value">%14 ₽</div>
            </div>

            <div class="info-row">
                <div class="info-label">Забронировано:</div>
                <div class="info-value">%15</div>
            </div>

            <div class="info-row">
                <div class="info-label">Оплачено:</div>
                <div class="info-value">%16</div>
            </div>
        </div>

        <div style="text-align: center; margin-top: 40px;">
            <div class="qr-placeholder">
                <div>QR-код<br/>билета</div>
            </div>
            <div style="color: #999; font-size: 12px;">Предъявите этот билет при посадке</div>
        </div>

        <div class="footer">
            <p><strong>Важная информация:</strong></p>
            <p>• Прибудьте на вокзал за 30 минут до отправления<br/>
            • Имейте при себе документ, указанный в билете<br/>
            • Этот билет действителен только для указанного места и времени<br/>
            • При утере билета обратитесь в кассу</p>
            <p style="margin-top: 20px;">
                Система бронирования железнодорожных билетов<br/>
                © 2025 Railway Booking System
            </p>
        </div>
    </div>
</body>
</html>
)";

    html = html
               .arg(ticket.ticketNumber)
               .arg(departureStationName)
               .arg(arrivalStationName)
               .arg(departureTime.toString("HH:mm"))
               .arg(departureTime.toString("dd.MM.yyyy"))
               .arg(arrivalTime.toString("HH:mm"))
               .arg(arrivalTime.toString("dd.MM.yyyy"))
               .arg(trainNumber)
               .arg(trainType)
               .arg(carriageNumber)
               .arg(seatNumber)
               .arg(ticket.passengerName)
               .arg(ticket.passengerDocument)
               .arg(ticket.price, 0, 'f', 2)
               .arg(ticket.bookedAt.toString("dd.MM.yyyy HH:mm"))
               .arg(ticket.paidAt.toString("dd.MM.yyyy HH:mm"));

    return html;
}
