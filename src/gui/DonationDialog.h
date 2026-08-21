#ifndef DONATIONDIALOG_H
#define DONATIONDIALOG_H

#include <QDialog>
#include <QWidget>

class QPushButton;

class DonationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DonationDialog(QWidget *parent = nullptr);
    ~DonationDialog() override = default;

private:
    void setupUI();
    QWidget* createQRCodeSection();
    QWidget* createInfoSection();
    QPushButton* createCloseButton();
};

#endif // DONATIONDIALOG_H