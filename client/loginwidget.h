#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "apiclient.h"

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);

signals:
    void loginSuccess();
    void registerRequested();

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onShowPasswordPressed();
    void onShowPasswordReleased();
    void onLoginSuccess(UserProfile user);
    void onLoginFailed(QString errorMessage);
    void on_createAccountButton_clicked();

private:
    void setupUi();

    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLineEdit* m_emailEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_showPasswordButton;
    QPushButton* m_loginButton;
    QPushButton* m_registerButton;
    QLabel* m_errorLabel;
};

#endif // LOGINWIDGET_H
