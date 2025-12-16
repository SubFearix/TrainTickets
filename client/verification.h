#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <QDialog>
#include <QString>

namespace Ui {
class Verification;
}

class Verification : public QDialog
{
    Q_OBJECT

public:
    explicit Verification(const QString& email, QWidget *parent = nullptr);
    ~Verification();

signals:
    void verificationSuccess();

private slots:
    void on_verifyButton_clicked();
    void on_resendButton_clicked();


    void onVerificationSuccess();
    void onVerificationFailed(QString errorMessage);
    void onResendSuccess();
    void onError(QString errorMessage);

private:
    Ui::Verification *ui;

    QString m_email;
};

#endif // VERIFICATION_H
