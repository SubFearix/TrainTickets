#include "verification.h"
#include "ui_verification.h"
#include <QMessageBox>
#include <QTimer>
#include "apiclient.h"

Verification::Verification(const QString &email, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Verification)
    , m_email(email)
{
    ui->setupUi(this);
    setWindowTitle("Подтверждение Email");

    ui->codeLineEdit->setFocus();
    ui->codeLineEdit->setMaxLength(6);

    connect(&ApiClient::instance(), &ApiClient::resendVerificationSuccess, this, &Verification:: onResendSuccess);
    connect(&ApiClient::instance(), &ApiClient::verificationSuccess, this, &Verification:: onVerificationSuccess);
    connect(&ApiClient::instance(), &ApiClient::verificationFailed, this, &Verification::onVerificationFailed);
    connect(&ApiClient::instance(), &ApiClient::error, this, &Verification::onError);

    connect(ui->codeLineEdit, &QLineEdit::returnPressed, this, &Verification::on_verifyButton_clicked);
}

Verification::~Verification()
{
    disconnect(&ApiClient::instance(), &ApiClient::resendVerificationSuccess, this, &Verification::onResendSuccess);
    disconnect(&ApiClient::instance(), &ApiClient::verificationSuccess, this, &Verification:: onVerificationSuccess);
    disconnect(&ApiClient::instance(), &ApiClient::verificationFailed, this, &Verification:: onVerificationFailed);
    disconnect(&ApiClient::instance(), &ApiClient::error, this, &Verification::onError);

    delete ui;
}

void Verification::on_verifyButton_clicked()
{
    QString enteredCode = ui->codeLineEdit->text().trimmed();

    if (enteredCode.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Пожалуйста, введите код подтверждения.");
        return;
    }

    ui->verifyButton->setEnabled(false);
    ui->verifyButton->setText("Проверка...");

    ApiClient::instance().verifyEmail(m_email, enteredCode);
}

void Verification::on_resendButton_clicked(){
    ui->resendButton->setEnabled(false);
    ui->resendButton->setText("Отправка...");

    ApiClient::instance().resendVerification(m_email);
}

void Verification::onResendSuccess(){
    ui->resendButton->setEnabled(true);
    ui->resendButton->setText("Отправить код повторно");
    QMessageBox::information(this, "Успех", "Код подтверждения отправлен повторно на вашу почту.");
}

void Verification:: onVerificationSuccess()
{
    ui->verifyButton->setEnabled(true);
    ui->verifyButton->setText("Подтвердить");

    QMessageBox::information(this, "Успех", "Email успешно подтвержден!");
    emit verificationSuccess();
    this->accept();
}

void Verification:: onVerificationFailed(QString errorMessage)
{
    ui->verifyButton->setEnabled(true);
    ui->verifyButton->setText("Подтвердить");

    QMessageBox:: critical(this, "Ошибка верификации", errorMessage);
    ui->codeLineEdit->clear();
    ui->codeLineEdit->setFocus();
}

void Verification::onError(QString errorMessage)
{
    ui->verifyButton->setEnabled(true);
    ui->verifyButton->setText("Подтвердить");
    ui->resendButton->setEnabled(true);
    ui->resendButton->setText("Отправить код повторно");

    QMessageBox:: critical(this, "Ошибка", errorMessage);
}
