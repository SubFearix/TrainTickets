#include "profilewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QRegularExpression>

ProfileWidget::ProfileWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&ApiClient::instance(), &ApiClient::profileReceived, this, &ProfileWidget::onProfileReceived);
    connect(&ApiClient::instance(), &ApiClient::passwordChanged, this, &ProfileWidget::onPasswordChanged);
    connect(&ApiClient::instance(), &ApiClient::passwordChangeError, this, &ProfileWidget::onError);
    connect(&ApiClient::instance(), &ApiClient::error, this, &ProfileWidget::onError);
}

void ProfileWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

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

    m_titleLabel = new QLabel("Мой профиль", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    headerLayout->addWidget(m_backButton);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_profileInfoWidget = new QWidget(this);
    QVBoxLayout* profileLayout = new QVBoxLayout(m_profileInfoWidget);

    QGroupBox* personalGroup = new QGroupBox("Личная информация", this);
    personalGroup->setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            border: 2px solid #2196F3;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    )");
    QVBoxLayout* personalLayout = new QVBoxLayout(personalGroup);
    personalLayout->setSpacing(15);

    QHBoxLayout* nameLayout = new QHBoxLayout();
    QLabel* nameIcon = new QLabel("👤", this);
    nameIcon->setStyleSheet("font-size: 24px;");
    m_nameLabel = new QLabel(this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setPointSize(16);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    nameLayout->addWidget(nameIcon);
    nameLayout->addWidget(m_nameLabel);
    nameLayout->addStretch();

    QHBoxLayout* emailLayout = new QHBoxLayout();
    QLabel* emailIcon = new QLabel("📧", this);
    emailIcon->setStyleSheet("font-size: 20px;");
    m_emailLabel = new QLabel(this);
    emailLayout->addWidget(emailIcon);
    emailLayout->addWidget(m_emailLabel);
    emailLayout->addStretch();

    m_verificationStatusLabel = new QLabel(this);

    personalLayout->addLayout(nameLayout);
    personalLayout->addLayout(emailLayout);
    personalLayout->addWidget(m_verificationStatusLabel);

    QGroupBox* statsGroup = new QGroupBox("Статистика аккаунта", this);
    statsGroup->setStyleSheet(personalGroup->styleSheet());
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
    statsLayout->setSpacing(12);

    QHBoxLayout* createdLayout = new QHBoxLayout();
    QLabel* createdIcon = new QLabel("📅", this);
    createdIcon->setStyleSheet("font-size: 18px;");
    QLabel* createdTextLabel = new QLabel("Зарегистрирован:", this);
    createdTextLabel->setStyleSheet("font-weight: bold; color: #666;");
    m_createdAtLabel = new QLabel(this);
    createdLayout->addWidget(createdIcon);
    createdLayout->addWidget(createdTextLabel);
    createdLayout->addWidget(m_createdAtLabel);
    createdLayout->addStretch();

    QHBoxLayout* loginLayout = new QHBoxLayout();
    QLabel* loginIcon = new QLabel("⏰", this);
    loginIcon->setStyleSheet("font-size: 18px;");
    QLabel* loginTextLabel = new QLabel("Последний вход:", this);
    loginTextLabel->setStyleSheet("font-weight: bold; color: #666;");
    m_lastLoginLabel = new QLabel(this);
    loginLayout->addWidget(loginIcon);
    loginLayout->addWidget(loginTextLabel);
    loginLayout->addWidget(m_lastLoginLabel);
    loginLayout->addStretch();

    statsLayout->addLayout(createdLayout);
    statsLayout->addLayout(loginLayout);

    m_changePasswordButton = new QPushButton("🔒 Сменить пароль", this);
    m_changePasswordButton->setMinimumHeight(45);
    m_changePasswordButton->setStyleSheet(R"(
        QPushButton {
            background-color: #FF9800;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #F57C00;
        }
    )");

    profileLayout->addWidget(personalGroup);
    profileLayout->addWidget(statsGroup);
    profileLayout->addSpacing(20);
    profileLayout->addWidget(m_changePasswordButton);
    profileLayout->addStretch();

    m_passwordChangeWidget = new QWidget(this);
    QVBoxLayout* passwordLayout = new QVBoxLayout(m_passwordChangeWidget);

    QGroupBox* passwordGroup = new QGroupBox("Смена пароля", this);
    passwordGroup->setStyleSheet(personalGroup->styleSheet());
    QFormLayout* passwordFormLayout = new QFormLayout(passwordGroup);
    passwordFormLayout->setSpacing(15);

    m_oldPasswordEdit = new QLineEdit(this);
    m_oldPasswordEdit->setPlaceholderText("Введите текущий пароль");
    m_oldPasswordEdit->setEchoMode(QLineEdit::Password);
    m_oldPasswordEdit->setMinimumHeight(35);

    m_newPasswordEdit = new QLineEdit(this);
    m_newPasswordEdit->setPlaceholderText("Минимум 8 символов");
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    m_newPasswordEdit->setMinimumHeight(35);

    m_confirmPasswordEdit = new QLineEdit(this);
    m_confirmPasswordEdit->setPlaceholderText("Повторите новый пароль");
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setMinimumHeight(35);

    passwordFormLayout->addRow("Текущий пароль:", m_oldPasswordEdit);
    passwordFormLayout->addRow("Новый пароль:", m_newPasswordEdit);
    passwordFormLayout->addRow("Подтверждение:", m_confirmPasswordEdit);

    m_passwordRequirementsLabel = new QLabel(
        "Требования к паролю:\n"
        "• Минимум 8 символов\n"
        "• Хотя бы одна заглавная буква\n"
        "• Хотя бы одна цифра\n"
        "• Хотя бы один специальный символ",
        this
        );
    m_passwordRequirementsLabel->setStyleSheet("color: #666; font-size: 11px;");

    m_passwordErrorLabel = new QLabel(this);
    m_passwordErrorLabel->setStyleSheet("color: red; font-weight: bold;");
    m_passwordErrorLabel->setWordWrap(true);
    m_passwordErrorLabel->hide();

    QHBoxLayout* passwordButtonsLayout = new QHBoxLayout();

    m_cancelPasswordButton = new QPushButton("Отмена", this);
    m_cancelPasswordButton->setMinimumSize(120, 40);
    m_cancelPasswordButton->setStyleSheet(R"(
        QPushButton {
            background-color: #757575;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #616161;
        }
    )");

    m_savePasswordButton = new QPushButton("Сохранить", this);
    m_savePasswordButton->setMinimumSize(120, 40);
    m_savePasswordButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");

    passwordButtonsLayout->addStretch();
    passwordButtonsLayout->addWidget(m_cancelPasswordButton);
    passwordButtonsLayout->addWidget(m_savePasswordButton);

    passwordLayout->addWidget(passwordGroup);
    passwordLayout->addWidget(m_passwordRequirementsLabel);
    passwordLayout->addWidget(m_passwordErrorLabel);
    passwordLayout->addSpacing(20);
    passwordLayout->addLayout(passwordButtonsLayout);
    passwordLayout->addStretch();

    m_passwordChangeWidget->hide();

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_profileInfoWidget);
    mainLayout->addWidget(m_passwordChangeWidget);

    connect(m_backButton, &QPushButton::clicked, this, &ProfileWidget::onBackClicked);
    connect(m_changePasswordButton, &QPushButton::clicked, this, &ProfileWidget::onChangePasswordClicked);
    connect(m_savePasswordButton, &QPushButton::clicked, this, &ProfileWidget::onSavePasswordClicked);
    connect(m_cancelPasswordButton, &QPushButton::clicked, this, &ProfileWidget::onCancelPasswordClicked);
}

void ProfileWidget::loadProfile()
{
    m_currentProfile = ApiClient::instance().getUserProfile();

    if (m_currentProfile.id > 0) {
        displayProfile();
    }

    ApiClient::instance().getProfile();

    showProfileInfo();
}

void ProfileWidget::displayProfile()
{
    m_nameLabel->setText(QString("%1 %2")
                             .arg(m_currentProfile.surname)
                             .arg(m_currentProfile.name));

    m_emailLabel->setText(m_currentProfile.email);

    if (m_currentProfile.isVerified) {
        m_verificationStatusLabel->setText("✅ Email подтверждён");
        m_verificationStatusLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
    } else {
        m_verificationStatusLabel->setText("⚠️ Email не подтверждён");
        m_verificationStatusLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
    }

    m_createdAtLabel->setText(m_currentProfile.createdAt.toString("dd.MM.yyyy"));

    if (m_currentProfile.lastLogin.isValid()) {
        m_lastLoginLabel->setText(m_currentProfile.lastLogin.toString("dd.MM.yyyy HH:mm"));
    } else {
        m_lastLoginLabel->setText("Нет данных");
    }
}

void ProfileWidget::onProfileReceived(UserProfile profile)
{
    m_currentProfile = profile;
    displayProfile();
}

void ProfileWidget::onBackClicked()
{
    showProfileInfo();
    emit backRequested();
}

void ProfileWidget::onChangePasswordClicked()
{
    showPasswordChangeForm();
}

void ProfileWidget::showProfileInfo()
{
    m_profileInfoWidget->show();
    m_passwordChangeWidget->hide();
    m_titleLabel->setText("Мой профиль");
}

void ProfileWidget::showPasswordChangeForm()
{
    m_profileInfoWidget->hide();
    m_passwordChangeWidget->show();
    m_titleLabel->setText("Смена пароля");

    m_oldPasswordEdit->clear();
    m_newPasswordEdit->clear();
    m_confirmPasswordEdit->clear();
    m_passwordErrorLabel->hide();
}

bool ProfileWidget::validatePasswordForm()
{
    QString oldPassword = m_oldPasswordEdit->text();
    QString newPassword = m_newPasswordEdit->text();
    QString confirmPassword = m_confirmPasswordEdit->text();

    if (oldPassword.isEmpty()) {
        m_passwordErrorLabel->setText("Введите текущий пароль");
        m_passwordErrorLabel->show();
        return false;
    }

    if (newPassword.isEmpty()) {
        m_passwordErrorLabel->setText("Введите новый пароль");
        m_passwordErrorLabel->show();
        return false;
    }

    if (newPassword.length() < 8) {
        m_passwordErrorLabel->setText("Пароль должен содержать минимум 8 символов");
        m_passwordErrorLabel->show();
        return false;
    }

    QRegularExpression upperCaseRegex("[A-Z]");
    if (!newPassword.contains(upperCaseRegex)) {
        m_passwordErrorLabel->setText("Пароль должен содержать хотя бы одну заглавную букву");
        m_passwordErrorLabel->show();
        return false;
    }

    QRegularExpression digitRegex("[0-9]");
    if (!newPassword.contains(digitRegex)) {
        m_passwordErrorLabel->setText("Пароль должен содержать хотя бы одну цифру");
        m_passwordErrorLabel->show();
        return false;
    }

    QRegularExpression specialCharRegex("[^a-zA-Z0-9\\s]");
    if (!newPassword.contains(specialCharRegex)) {
        m_passwordErrorLabel->setText("Пароль должен содержать хотя бы один специальный символ");
        m_passwordErrorLabel->show();
        return false;
    }

    if (newPassword != confirmPassword) {
        m_passwordErrorLabel->setText("Пароли не совпадают");
        m_passwordErrorLabel->show();
        return false;
    }

    if (oldPassword == newPassword) {
        m_passwordErrorLabel->setText("Новый пароль должен отличаться от текущего");
        m_passwordErrorLabel->show();
        return false;
    }

    return true;
}

void ProfileWidget::onSavePasswordClicked()
{
    m_passwordErrorLabel->hide();

    if (!validatePasswordForm()) {
        return;
    }

    m_savePasswordButton->setEnabled(false);
    m_savePasswordButton->setText("Сохранение...");

    ApiClient::instance().changePassword(
        m_oldPasswordEdit->text(),
        m_newPasswordEdit->text()
        );
}

void ProfileWidget::onCancelPasswordClicked()
{
    showProfileInfo();
}

void ProfileWidget::onPasswordChanged()
{
    m_savePasswordButton->setEnabled(true);
    m_savePasswordButton->setText("Сохранить");

    QMessageBox::information(this, "Успешно",
                             "Пароль успешно изменён!\n\n"
                             "При следующем входе используйте новый пароль.");

    showProfileInfo();
}

void ProfileWidget::onError(QString errorMessage)
{
    m_savePasswordButton->setEnabled(true);
    m_savePasswordButton->setText("Сохранить");

    if (m_passwordChangeWidget->isVisible()) {
        if (errorMessage.contains("password", Qt::CaseInsensitive) ||
            errorMessage.contains("пароль", Qt::CaseInsensitive) ||
            errorMessage.contains("incorrect", Qt::CaseInsensitive) ||
            errorMessage.contains("неверн", Qt::CaseInsensitive) ||
            errorMessage.contains("Wrong", Qt::CaseInsensitive) ||
            errorMessage.isEmpty()) {

            m_passwordErrorLabel->setText("Неверный текущий пароль. Проверьте правильность ввода.");
            m_passwordErrorLabel->show();
        } else {
            m_passwordErrorLabel->setText(errorMessage);
            m_passwordErrorLabel->show();
        }
    }
}
