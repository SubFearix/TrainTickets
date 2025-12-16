#ifndef CONFIG_H
#define CONFIG_H
#include <QSettings>
#include <QString>

class EmailConfig{
private:
    EmailConfig() {}
    QString m_username;
    QString m_password;
    QString m_smtpHost;
    int m_smtpPort;

public:
    static EmailConfig& instance() {
        static EmailConfig instance;
        return instance;
    }

    bool loadConfig(const QString& configPath = "config/email.conf") {
        QSettings settings(configPath, QSettings::IniFormat);

        m_smtpHost = settings.value("smtp/host", "smtp.gmail.com").toString();
        m_smtpPort = settings.value("smtp/port", 465).toInt();
        m_username = settings.value("smtp/username", "").toString();
        m_password = settings.value("smtp/password", "").toString();

        return !m_username.isEmpty() && !m_password.isEmpty();
    }

    QString username() const { return m_username; }
    QString password() const { return m_password; }
    QString smtpHost() const { return m_smtpHost; }
    int smtpPort() const { return m_smtpPort; }
};

#endif // CONFIG_H
