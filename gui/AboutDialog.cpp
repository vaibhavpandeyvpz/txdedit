#include "AboutDialog.h"
#include "gui/version.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>
#include <QLabel>
#include <QPushButton>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("About");
    setFixedSize(450, 420);
    setModal(true);
    
    setupUI();
}

QString AboutDialog::getLogoPath(const QString& logoName) const {
    // Use Qt resource system
    QString resourcePath = ":/logos/" + logoName;
    if (QFile::exists(resourcePath)) {
        return resourcePath;
    }
    return QString();
}

void AboutDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Add stretch at top to center content vertically
    mainLayout->addStretch();
    
    // Title
    QLabel* titleLabel = new QLabel("TXD Edit", this);
    titleLabel->setStyleSheet("QLabel { font-size: 28px; font-weight: bold; color: #ffffff; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Version
    QLabel* versionLabel = new QLabel(QString("Version %1").arg(TXDEDIT_VERSION_STRING), this);
    versionLabel->setStyleSheet("QLabel { font-size: 12px; color: #ffffff; }");
    versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(versionLabel);
    
    mainLayout->addSpacing(10);
    
    // Game logos
    QHBoxLayout* logosLayout = new QHBoxLayout();
    logosLayout->setSpacing(12);
    logosLayout->setAlignment(Qt::AlignCenter);
    
    QLabel* gta3Logo = new QLabel(this);
    QString gta3Path = getLogoPath("gta3.png");
    if (!gta3Path.isEmpty()) {
        QPixmap gta3Pixmap(gta3Path);
        // Scale to 48px height, maintaining aspect ratio
        QPixmap scaledGta3 = gta3Pixmap.scaledToHeight(48, Qt::SmoothTransformation);
        gta3Logo->setPixmap(scaledGta3);
        gta3Logo->setFixedHeight(48);
        gta3Logo->setMinimumWidth(scaledGta3.width());
    }
    gta3Logo->setAlignment(Qt::AlignCenter);
    gta3Logo->setScaledContents(false);
    logosLayout->addWidget(gta3Logo);
    
    QLabel* vcLogo = new QLabel(this);
    QString vcPath = getLogoPath("gtavc.png");
    if (!vcPath.isEmpty()) {
        QPixmap vcPixmap(vcPath);
        // Scale to 48px height, maintaining aspect ratio
        QPixmap scaledVc = vcPixmap.scaledToHeight(48, Qt::SmoothTransformation);
        vcLogo->setPixmap(scaledVc);
        vcLogo->setFixedHeight(48);
        vcLogo->setMinimumWidth(scaledVc.width());
    }
    vcLogo->setAlignment(Qt::AlignCenter);
    vcLogo->setScaledContents(false);
    logosLayout->addWidget(vcLogo);
    
    QLabel* saLogo = new QLabel(this);
    QString saPath = getLogoPath("gtasa.png");
    if (!saPath.isEmpty()) {
        QPixmap saPixmap(saPath);
        // Scale to 48px height, maintaining aspect ratio
        QPixmap scaledSa = saPixmap.scaledToHeight(48, Qt::SmoothTransformation);
        saLogo->setPixmap(scaledSa);
        saLogo->setFixedHeight(48);
        saLogo->setMinimumWidth(scaledSa.width());
    }
    saLogo->setAlignment(Qt::AlignCenter);
    saLogo->setScaledContents(false);
    logosLayout->addWidget(saLogo);
    
    mainLayout->addLayout(logosLayout);
    
    mainLayout->addSpacing(12);
    
    // Description
    QLabel* descLabel = new QLabel("A visual editor for viewing and editing TXD files from GTA III, GTA Vice City and GTA San Andreas.", this);
    descLabel->setStyleSheet("QLabel { font-size: 11px; color: #b0b0b0; }");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);
    
    mainLayout->addSpacing(8);
    
    // Developer
    QLabel* devLabel = new QLabel("Developed by Vaibhav Pandey (VPZ).", this);
    devLabel->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #ffffff; }");
    devLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(devLabel);
    
    mainLayout->addSpacing(8);
    
    // Links
    QHBoxLayout* linksLayout = new QHBoxLayout();
    linksLayout->setSpacing(10);
    linksLayout->setAlignment(Qt::AlignCenter);
    
    auto createLink = [this](const QString& text, const QString& url) -> QLabel* {
        QLabel* link = new QLabel(QString("<a href=\"%1\" style=\"color: #00aaff; text-decoration: none;\">%2</a>").arg(url, text), this);
        link->setOpenExternalLinks(true);
        link->setStyleSheet("QLabel { font-size: 12px; }");
        return link;
    };
    
    linksLayout->addWidget(createLink("Website", "https://vaibhavpandey.com/"));
    linksLayout->addWidget(createLink("YouTube", "https://www.youtube.com/@vaibhavpandeyvpz"));
    linksLayout->addWidget(createLink("GitHub", "https://github.com/vaibhavpandeyvpz/txdedit"));
    linksLayout->addWidget(createLink("Email", "mailto:contact@vaibhavpandey.com"));
    linksLayout->addWidget(createLink("Issues", "https://github.com/vaibhavpandeyvpz/txdedit/issues"));
    
    mainLayout->addLayout(linksLayout);
    
    mainLayout->addSpacing(8);
    
    // Disclaimer
    QLabel* disclaimerLabel = new QLabel("All names and logos are property of their respective owners and are used for illustration purposes only.", this);
    disclaimerLabel->setStyleSheet("QLabel { font-size: 9px; color: #888888; }");
    disclaimerLabel->setAlignment(Qt::AlignCenter);
    disclaimerLabel->setWordWrap(true);
    mainLayout->addWidget(disclaimerLabel);
    
    // Add stretch before button to center content
    mainLayout->addStretch();
    
    // Close button with X icon
    QPushButton* closeBtn = new QPushButton("✕ Close", this);
    closeBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #ff8800; "
        "color: #ffffff; "
        "border: none; "
        "padding: 8px 24px; "
        "font-size: 12px; "
        "font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "background-color: #ffaa00; "
        "} "
        "QPushButton:pressed { "
        "background-color: #ff6600; "
        "}"
    );
    closeBtn->setFixedHeight(36);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Set dark background
    setStyleSheet("QDialog { background-color: #1a1a1a; }");
}

