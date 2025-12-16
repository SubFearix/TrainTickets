#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

const QString API_URL = "http://localhost:8080/api";
const QString API_HOST = "localhost";
const quint16 API_PORT = 8080;

const int NETWORK_TIMEOUT = 30000;
const int BOOKING_TIMEOUT = 15 * 60 * 1000;

const QString APP_NAME = "Railway Ticket Booking System";
const QString APP_VERSION = "1.0.0";
const QString APP_ORGANIZATION = "Railway Inc.";

// Настройки UI
const int MIN_WINDOW_WIDTH = 1024;
const int MIN_WINDOW_HEIGHT = 768;

// Настройки валидации
const int MIN_PASSWORD_LENGTH = 8;
const int VERIFICATION_CODE_LENGTH = 6;

// Пути к ресурсам
const QString ICONS_PATH = ":/icons/";
const QString IMAGES_PATH = ":/images/";

#endif // CONFIG_H
