#include "registration.h"
#include "ui_registration.h"
#include <QDebug>
#include <QRegularExpression>
#include <QMessageBox>
#include "verification.h"
#include "apiclient.h"
#include "mainwindow.h"

using namespace std;

Registration::Registration(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Registration)
    , m_pendingUserId(0)
{
    ui->setupUi(this);

    this->setMinimumSize(500, 700);
    this->resize(500, 750);

    connect(ui->nameEdit, &QLineEdit::returnPressed, this, &Registration::focusNextField);
    connect(ui->surnameEdit, &QLineEdit::returnPressed, this, &Registration::focusNextField);
    connect(ui->emailEditReg, &QLineEdit::returnPressed, this, &Registration::focusNextField);
    connect(ui->emailEditReg, &QLineEdit::editingFinished, this, &Registration::handleEmailReturnPressed);
    connect(ui->passwordEditReg, &QLineEdit::returnPressed, this, &Registration::focusNextField);
    connect(ui->passwordEditRegAgain, &QLineEdit::returnPressed, ui->regButton, &QPushButton::click);

    ui->showPasswordReg->setText("");
    ui->showPasswordReg->setIcon(QIcon("icons/closed_eye.png"));
    ui->showPasswordReg->setIconSize(QSize(20, 20));

    connect(&ApiClient::instance(), &ApiClient::registerSuccess, this, &Registration::onRegisterSuccess);
    connect(&ApiClient::instance(), &ApiClient::registerFailed, this, &Registration::onRegisterFailed);
    connect(ui->backButton, &QPushButton::clicked, this, &Registration:: close);
}

Registration::~Registration()
{
    disconnect(&ApiClient::instance(), &ApiClient::registerSuccess, this, &Registration::onRegisterSuccess);
    disconnect(&ApiClient::instance(), &ApiClient::registerFailed, this, &Registration:: onRegisterFailed);

    delete ui;
}


void Registration::on_regButton_clicked()
{
    bool requirements = true;
    QString name = ui->nameEdit->text();
    QString surname = ui->surnameEdit->text();
    QString email = ui->emailEditReg->text().trimmed();
    QString password = ui->passwordEditReg->text();
    QString repeatPassword = ui->passwordEditRegAgain->text();

    ui->noName->setText("");
    ui->noSurname->setText("");
    ui->noEmail->setText("");
    ui->checkEmail->setText("");
    ui->passwordRequirements->setText("");
    ui->passwordsNotMatch->setText("");

    if (name.isEmpty()) {
        ui->noName->setText("Это поле обязательно для заполнения");
        return;
    }

    if (surname.isEmpty()) {
        ui->noSurname->setText("Это поле обязательно для заполнения");
        return;
    }

    QRegularExpression emailRegex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch emailMatch = emailRegex.match(email);
    if (!emailMatch.hasMatch() || emailMatch.capturedLength() != email.length()) {
        ui->checkEmail->setText("Некорректный формат Email");
        ui->checkEmail->setStyleSheet("color: red;");
        return;
    }

    QString statusMessage = "";
    qDebug() << password << " " << repeatPassword;
    if (password.length() < 8){
        statusMessage += "- Длина должна быть не менее 8 символов.\n";
        requirements = false;
    }

    QRegularExpression upperCaseRegex("[A-Z]");
    if (!password.contains(upperCaseRegex)) {
        statusMessage += "- Должна быть хотя бы одна заглавная буква.\n";
        requirements = false;
    }

    QRegularExpression digitRegex("[0-9]");
    if (!password.contains(digitRegex)) {
        statusMessage += "- Должна быть хотя бы одна цифра.\n";
        requirements = false;
    }

    QRegularExpression specialCharRegex("[^a-zA-Z0-9\\s]");
    if (!password.contains(specialCharRegex)) {
        statusMessage += "- Должен быть хотя бы один специальный символ.\n";
        requirements = false;
    }

    if (!statusMessage.isEmpty()) {
        ui->passwordRequirements->setText(statusMessage);
    }

    if (statusMessage.isEmpty()){
        if (password != repeatPassword){
            ui->passwordsNotMatch->setText("Пароли не совпадают");
            requirements = false;
        }
    }

    if (requirements){
        ui->regButton->setEnabled(false);
        ui->regButton->setText("Регистрация...");

        m_pendingEmail = email;
        ApiClient::instance().registerUser(name, surname, email, password);
    }
}

void Registration::focusNextField(){
    this->focusNextChild();
}

void Registration::on_showPasswordReg_pressed()
{
    ui->passwordEditReg->setEchoMode(QLineEdit::Normal);
    ui->showPasswordReg->setIcon(QIcon("icons/open_eye.png"));
}

void Registration::on_showPasswordReg_released()
{
    ui->passwordEditReg->setEchoMode(QLineEdit::Password);
    ui->showPasswordReg->setIcon(QIcon("icons/closed_eye.png"));
}

void Registration::handleEmailReturnPressed(){
    QString email = ui->emailEditReg->text().trimmed();

    ui->noEmail->hide();
    ui->checkEmail->hide();

    if (email.isEmpty()) {
        ui->noEmail->setText("Это поле обязательно для заполнения");
        ui->noEmail->show();
        return;
    }

    QRegularExpression emailRegex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = emailRegex.match(email);

    if (match.hasMatch() && match.capturedLength() == email.length())
    {
        ui->passwordEditReg->setFocus();
    }
    else
    {
        ui->checkEmail->setText("Некорректный формат Email");
        ui->checkEmail->setStyleSheet("color: red;");
        ui->checkEmail->show();
        ui->emailEditReg->setFocus();
    }
}

void Registration::handleVerificationSuccess(){
    ui->nameEdit->clear();
    ui->surnameEdit->clear();
    ui->emailEditReg->clear();
    ui->passwordEditReg->clear();
    ui->passwordEditRegAgain->clear();
    ui->passwordRequirements->clear();
    ui->passwordsNotMatch->clear();
    ui->checkEmail->clear();

    emit registrationComplete();
    this->close();
}

void Registration::onRegisterSuccess(int userId, QString email, bool requiresVerification)
{
    ui->regButton->setEnabled(true);
    ui->regButton->setText("Зарегистрироваться");

    m_pendingUserId = userId;

    if (requiresVerification) {
        QMessageBox::information(this, "Успех", "Регистрация успешна.  Пожалуйста, проверьте вашу почту для подтверждения.");

        Verification* verifWindow = new Verification(m_pendingEmail, this);
        connect(verifWindow, &Verification::verificationSuccess, this, &Registration::handleVerificationSuccess);
        verifWindow->open();
    } else {
        handleVerificationSuccess();
    }
}

void Registration::onRegisterFailed(QString errorMessage)
{
    ui->regButton->setEnabled(true);
    ui->regButton->setText("Зарегистрироваться");

    QMessageBox::warning(this, "Ошибка регистрации", errorMessage);
    qDebug() << "Registration failed:" << errorMessage;
}
