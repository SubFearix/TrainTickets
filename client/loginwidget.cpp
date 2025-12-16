#include "loginwidget.h"
#include "registration.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget{parent}
{
    setupUi();

    connect(&ApiClient::instance(), &ApiClient::loginSuccess, this, &LoginWidget::onLoginSuccess);
    connect(&ApiClient::instance(), &ApiClient::loginFailed, this, &LoginWidget::onLoginFailed);
}

void LoginWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    m_titleLabel = new QLabel("Добро пожаловать!", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_subtitleLabel = new QLabel("Система бронирования железнодорожных билетов", this);
    QFont subtitleFont = m_subtitleLabel->font();
    subtitleFont.setPointSize(13);
    m_subtitleLabel->setFont(subtitleFont);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("color: #666;");

    QWidget* formWidget = new QWidget(this);
    formWidget->setMaximumWidth(400);
    formWidget->setStyleSheet(R"(
        QWidget {
            background-color: white;
            border-radius: 10px;
        }
    )");

    QVBoxLayout* formLayout = new QVBoxLayout(formWidget);
    formLayout->setContentsMargins(30, 30, 30, 30);
    formLayout->setSpacing(15);

    QLabel* emailLabel = new QLabel("Email:", this);
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("Введите ваш email");
    m_emailEdit->setMinimumHeight(40);
    m_emailEdit->setStyleSheet(R"(
        QLineEdit {
            border: 2px solid #ddd;
            border-radius: 5px;
            padding: 5px 10px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #2196F3;
        }
    )");

    QLabel* passwordLabel = new QLabel("Пароль:", this);

    QHBoxLayout* passwordLayout = new QHBoxLayout();
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Введите пароль");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumHeight(40);
    m_passwordEdit->setStyleSheet(m_emailEdit->styleSheet());

    m_showPasswordButton = new QPushButton(this);
    m_showPasswordButton->setIcon(QIcon("icons/closed_eye.png"));
    m_showPasswordButton->setIconSize(QSize(20, 20));
    m_showPasswordButton->setMaximumWidth(40);
    m_showPasswordButton->setMinimumHeight(40);
    m_showPasswordButton->setStyleSheet(R"(
        QPushButton {
            background-color: #f5f5f5;
            border: 2px solid #ddd;
            border-radius: 5px;
        }
        QPushButton:hover {
                background-color: #e0e0e0;
        }
        QPushButton:pressed {
            background-color: #E3F2FD;
            border-color: #2196F3;
        }
    )");

    passwordLayout->addWidget(m_passwordEdit);
    passwordLayout->addWidget(m_showPasswordButton);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: #f44336; font-size: 13px;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_loginButton = new QPushButton("Войти", this);
    m_loginButton->setMinimumHeight(45);
    m_loginButton->setCursor(Qt::PointingHandCursor);
    m_loginButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1976D2;
        }
        QPushButton:pressed {
            background-color: #0D47A1;
        }
    )");

    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #ddd;");

    m_registerButton = new QPushButton("Создать аккаунт", this);
    m_registerButton->setMinimumHeight(45);
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #2E7D32;
        }
    )");

    formLayout->addWidget(emailLabel);
    formLayout->addWidget(m_emailEdit);
    formLayout->addWidget(passwordLabel);
    formLayout->addLayout(passwordLayout);
    formLayout->addWidget(m_errorLabel);
    formLayout->addSpacing(10);
    formLayout->addWidget(m_loginButton);
    formLayout->addWidget(separator);
    formLayout->addWidget(m_registerButton);

    mainLayout->addStretch();
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_subtitleLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(formWidget, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    connect(m_loginButton, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    connect(m_registerButton, &QPushButton::clicked, this, &LoginWidget::onRegisterClicked);
    connect(m_showPasswordButton, &QPushButton::pressed, this, &LoginWidget::onShowPasswordPressed);
    connect(m_showPasswordButton, &QPushButton::released, this, &LoginWidget::onShowPasswordReleased);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLoginClicked);
    connect(m_emailEdit, &QLineEdit::returnPressed, [this]() { m_passwordEdit->setFocus(); });
}

void LoginWidget::onLoginClicked()
{
    m_errorLabel->hide();

    QString email = m_emailEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("Заполните все поля");
        m_errorLabel->show();
        return;
    }

    m_loginButton->setEnabled(false);
    m_loginButton->setText("Вход...");

    ApiClient::instance().login(email, password);
}

void LoginWidget::onRegisterClicked()
{
    emit registerRequested();
}

void LoginWidget::onLoginSuccess(UserProfile user)
{
    Q_UNUSED(user);

    m_loginButton->setEnabled(true);
    m_loginButton->setText("Войти");

    m_emailEdit->clear();
    m_passwordEdit->clear();
    m_errorLabel->hide();

    emit loginSuccess();
}

void LoginWidget::onLoginFailed(QString errorMessage)
{
    m_loginButton->setEnabled(true);
    m_loginButton->setText("Войти");

    m_errorLabel->setText(errorMessage);
    m_errorLabel->show();
}

void LoginWidget::on_createAccountButton_clicked()
{
    Registration* reg = new Registration(this);
    reg->setAttribute(Qt::WA_DeleteOnClose);
    connect(reg, &Registration::registrationComplete, this, [this]() {
    });
    reg->show();
}

void LoginWidget::onShowPasswordPressed()
{
    m_passwordEdit->setEchoMode(QLineEdit::Normal);
    m_showPasswordButton->setIcon(QIcon("icons/open_eye.png"));
}

void LoginWidget::onShowPasswordReleased()
{
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_showPasswordButton->setIcon(QIcon("icons/closed_eye.png"));
}
