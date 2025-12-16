#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include "apiclient.h"

class ProfileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileWidget(QWidget *parent = nullptr);

    void loadProfile();
    void displayProfile();

signals:
    void backRequested();

private slots:
    void onProfileReceived(UserProfile profile);
    void onBackClicked();
    void onChangePasswordClicked();
    void onSavePasswordClicked();
    void onCancelPasswordClicked();
    void onPasswordChanged();
    void onError(QString errorMessage);

private:
    void setupUi();
    void showProfileInfo();
    void showPasswordChangeForm();
    bool validatePasswordForm();

    QLabel* m_titleLabel;
    QLabel* m_nameLabel;
    QLabel* m_emailLabel;
    QLabel* m_createdAtLabel;
    QLabel* m_lastLoginLabel;
    QLabel* m_verificationStatusLabel;
    QPushButton* m_backButton;
    QPushButton* m_changePasswordButton;

    QWidget* m_profileInfoWidget;
    QWidget* m_passwordChangeWidget;
    QLineEdit* m_oldPasswordEdit;
    QLineEdit* m_newPasswordEdit;
    QLineEdit* m_confirmPasswordEdit;
    QPushButton* m_savePasswordButton;
    QPushButton* m_cancelPasswordButton;
    QLabel* m_passwordErrorLabel;
    QLabel* m_passwordRequirementsLabel;

    UserProfile m_currentProfile;
};

#endif // PROFILEWIDGET_H
