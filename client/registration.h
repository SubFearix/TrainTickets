#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <QWidget>
#include <QString>

namespace Ui {
class Registration;
}

class Registration : public QWidget
{
    Q_OBJECT

public:
    explicit Registration(QWidget *parent = nullptr);
    ~Registration();

signals:
    void registrationComplete();

private slots:
    void on_regButton_clicked();
    void focusNextField();

    void on_showPasswordReg_pressed();
    void on_showPasswordReg_released();

    void handleEmailReturnPressed();
    void handleVerificationSuccess();

    void onRegisterSuccess(int userId, QString email, bool requiresVerification);
    void onRegisterFailed(QString errorMessage);

private:
    Ui::Registration *ui;

    QString m_pendingEmail;
    int m_pendingUserId;
};
#endif // REGISTRATION_H
