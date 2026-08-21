#include "DonationDialog.h"
#include "../app/AppMeta.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

DonationDialog::DonationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Donate"));
    setFixedSize(480, 680);
    setStyleSheet("QDialog { background-color: #2b2b2b; }");
    
    setupUI();
}

void DonationDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(25, 25, 25, 30);

    // Title
    QLabel *titleLabel = new QLabel(tr("Support %1 Development")
                                        .arg(QLatin1String(AppMeta::NAME)));
    titleLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(titleLabel);

    // Main text
    QLabel *mainText = new QLabel(tr(
        "%1 is a true Vibe coding project – developed with a lot of passion and using GitHub Copilot Pro+.\n\n"
        "Behind it, however, lie countless thoughts, thousands of hours of work, and countless debugging sessions.\n\n"
        "Besides the development work, I also have to cover the Copilot Pro+ subscription costs.\n\n"
        "If you like this software, I would be very grateful for your support!"
    ).arg(QLatin1String(AppMeta::NAME)));
    mainText->setStyleSheet("color: white; font-size: 13px;");
    mainText->setWordWrap(true);
    mainText->setAlignment(Qt::AlignLeft);
    layout->addWidget(mainText);

    // Your support helps
    QLabel *benefitsLabel = new QLabel(
        "<div style='color: white; font-size: 12px;'>"
        "<b>Your support helps:</b><br/>"
        "• Add new features and improvements<br/>"
        "• Fix bugs and enhance stability<br/>"
        "• Maintain regular updates<br/>"
        "• Keep the software free and open-source<br/>"
        "• Cover monthlyCopilot Pro+ subscription costs (about $40)"
        "</div>"
    );
    benefitsLabel->setWordWrap(true);
    benefitsLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(benefitsLabel);

    // QR-Code Sektion hinzufügen
    layout->addWidget(createQRCodeSection());

    // Info Sektion hinzufügen  
    layout->addWidget(createInfoSection());

    // Close Button hinzufügen
    layout->addWidget(createCloseButton());
}

QWidget* DonationDialog::createQRCodeSection()
{
    // QR-Code Container als separates Widget mit fester Geometrie
    QWidget *qrContainer = new QWidget();
    qrContainer->setFixedHeight(180);
    qrContainer->setStyleSheet("QWidget { background-color: transparent; }");
    
    QVBoxLayout *qrContainerLayout = new QVBoxLayout(qrContainer);
    qrContainerLayout->setContentsMargins(0, 20, 0, 20);
    qrContainerLayout->setSpacing(0);
    
    // QR-Code als QTextEdit für bessere Kontrolle über Monospace-Darstellung
    QTextEdit *qrCodeDisplay = new QTextEdit();
    qrCodeDisplay->setFixedSize(160, 140);
    qrCodeDisplay->setReadOnly(true);
    qrCodeDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    qrCodeDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    qrCodeDisplay->setStyleSheet(
        "QTextEdit {"
        "background-color: white;"
        "border: 2px solid #ddd;"
        "border-radius: 8px;"
        "padding: 8px;"
        "font-family: 'Courier New', monospace;"
        "font-size: 6px;"
        "line-height: 6px;"
        "color: black;"
        "}"
    );
    
    // ASCII QR-Code Text ohne HTML
    QString qrCodeText = 
        "█████████████████████████\n"
        "█ ▄▄▄▄▄ █▀█ █ ▄▄▄▄▄ █\n"
        "█ █   █ █▀▀▀█ █   █ █\n"
        "█ █▄▄▄█ █ ▀ █ █▄▄▄█ █\n"
        "█▄▄▄▄▄▄▄█▄▀▄█▄▄▄▄▄▄▄█\n"
        "█▄▄█▄▀▄▄  ▀█▀ ▀█▄█▄ █\n"
        "█ ▄▀▄ ▄▄█▀▀██▀▀█▀▀▄▄█\n"
        "█▄ ▀▄▄▄▄▀ ▄▄▀ ▄▄▄ ▀ █\n"
        "█ ▄▄▄▄▄ █▄ █ █ ▄ ██▄█\n"
        "█ █   █ █▀▀▀█▄▄▄▄▀█ █\n"
        "█ █▄▄▄█ █▀▄ ▀▀▀▀▀▄▀▄█\n"
        "█▄▄▄▄▄▄▄█▄▄██▄█▄▄█▄▄█\n"
        "█████████████████████████";
    
    qrCodeDisplay->setPlainText(qrCodeText);
    
    // QR-Code horizontal zentrieren
    QHBoxLayout *qrCenterLayout = new QHBoxLayout();
    qrCenterLayout->addStretch();
    qrCenterLayout->addWidget(qrCodeDisplay);
    qrCenterLayout->addStretch();
    
    qrContainerLayout->addLayout(qrCenterLayout);
    
    return qrContainer;
}

QWidget* DonationDialog::createInfoSection()
{
    QWidget *infoContainer = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoContainer);
    infoLayout->setSpacing(10);
    
    // PayPal Info
    QLabel *paypalLabel = new QLabel(
        "<div style='color: white; font-size: 12px;'>"
        "<b>PayPal Donation:</b><br/>"
        "Scan the QR-Code above or visit:<br/>"
        "paypal.me/yourusername"
        "</div>"
    );
    paypalLabel->setWordWrap(true);
    paypalLabel->setAlignment(Qt::AlignLeft);
    paypalLabel->setContentsMargins(0, 10, 0, 10);
    infoLayout->addWidget(paypalLabel);

    // Dankeschön
    QLabel *thanksLabel = new QLabel(tr("Thank you for your support!"));
    thanksLabel->setStyleSheet("color: white; font-size: 12px;");
    thanksLabel->setWordWrap(true);
    thanksLabel->setAlignment(Qt::AlignLeft);
    infoLayout->addWidget(thanksLabel);
    
    return infoContainer;
}

QPushButton* DonationDialog::createCloseButton()
{
    QPushButton *closeButton = new QPushButton(tr("Close"));
    closeButton->setStyleSheet(
        "QPushButton {"
        "background-color: #555; color: white;"
        "border: none; padding: 10px 20px; border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "background-color: #666;"
        "}"
    );
    
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    return closeButton;
}