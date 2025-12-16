#ifndef EMAILCONFIG_H
#define EMAILCONFIG_H

#include <QString>
#include <QSettings>
#include <QCoreApplication>
#include <QFile>

class EmailConfig {
public:
    static EmailConfig& instance(){
        static EmailConfig inst;
        return inst;
    }

    bool loadConfig(const QString& configPath = ""){
        QString path = configPath;

        if (path.isEmpty()) {
            path = QCoreApplication::applicationDirPath() + "/config/email.conf";
        }

        qDebug() << "Loading email config from:" << path;

        if (!QFile::exists(path)) {
            qWarning() << "Email config file not found:" << path;
            return false;
        }

        QSettings settings(path, QSettings::IniFormat);

        m_smtpHost = settings.value("email/smtp_host", "smtp.gmail.com").toString();
        m_smtpPort = settings.value("email/smtp_port", 465).toInt();
        m_username = settings.value("email/username", "").toString();
        m_password = settings.value("email/password", "").toString();

        return !m_username.isEmpty() && !m_password.isEmpty();
    }

    QString smtpHost() const { return m_smtpHost; }
    int smtpPort() const { return m_smtpPort; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }

private:
    EmailConfig() {}
    QString m_smtpHost;
    int m_smtpPort;
    QString m_username;
    QString m_password;
};

#endif // EMAILCONFIG_H
