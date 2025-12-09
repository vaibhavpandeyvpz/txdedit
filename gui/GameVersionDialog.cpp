#include "GameVersionDialog.h"
#include <QFile>
#include <QDir>
#include <QPixmap>
#include <QIcon>
#include <QSizePolicy>

GameVersionDialog::GameVersionDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Select GTA Game Version");
    setModal(true);
    setMinimumSize(500, 200);
    setupUI();
}

void GameVersionDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Instruction label
    instructionLabel = new QLabel("Please select the GTA game version for this TXD file:", this);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet("QLabel { color: #e0e0e0; font-size: 14px; font-weight: bold; }");
    mainLayout->addWidget(instructionLabel);
    
    // Buttons layout
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(20);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    
    // GTA III button
    gta3Button = new QPushButton(this);
    gta3Button->setMinimumSize(140, 100);
    gta3Button->setMaximumSize(140, 100);
    gta3Button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    gta3Button->setStyleSheet(
        "QPushButton {"
        "    border: 2px solid #FF6B6B;"
        "    border-radius: 8px;"
        "    background-color: #2a2a2a;"
        "    padding: 10px;"
        "}"
        "QPushButton:hover {"
        "    border: 3px solid #FF6B6B;"
        "    background-color: #3a3a3a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1a1a1a;"
        "}"
    );
    
    QString gta3Logo = getLogoPath("gta3.png");
    if (!gta3Logo.isEmpty() && QFile::exists(gta3Logo)) {
        QPixmap pixmap(gta3Logo);
        QIcon icon(pixmap.scaled(120, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        gta3Button->setIcon(icon);
        gta3Button->setIconSize(QSize(120, 48));
    } else {
        gta3Button->setText("GTA:III");
        gta3Button->setStyleSheet(gta3Button->styleSheet() + "QPushButton { color: #FF6B6B; font-size: 16px; font-weight: bold; }");
    }
    
    connect(gta3Button, &QPushButton::clicked, this, &GameVersionDialog::onGTA3Selected);
    buttonsLayout->addWidget(gta3Button);
    
    // GTA VC button
    gtavcButton = new QPushButton(this);
    gtavcButton->setMinimumSize(140, 100);
    gtavcButton->setMaximumSize(140, 100);
    gtavcButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    gtavcButton->setStyleSheet(
        "QPushButton {"
        "    border: 2px solid #4ECDC4;"
        "    border-radius: 8px;"
        "    background-color: #2a2a2a;"
        "    padding: 10px;"
        "}"
        "QPushButton:hover {"
        "    border: 3px solid #4ECDC4;"
        "    background-color: #3a3a3a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1a1a1a;"
        "}"
    );
    
    QString gtavcLogo = getLogoPath("gtavc.png");
    if (!gtavcLogo.isEmpty() && QFile::exists(gtavcLogo)) {
        QPixmap pixmap(gtavcLogo);
        QIcon icon(pixmap.scaled(120, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        gtavcButton->setIcon(icon);
        gtavcButton->setIconSize(QSize(120, 48));
    } else {
        gtavcButton->setText("GTA:VC");
        gtavcButton->setStyleSheet(gtavcButton->styleSheet() + "QPushButton { color: #4ECDC4; font-size: 16px; font-weight: bold; }");
    }
    
    connect(gtavcButton, &QPushButton::clicked, this, &GameVersionDialog::onGTAVCSelected);
    buttonsLayout->addWidget(gtavcButton);
    
    // GTA SA button
    gtasaButton = new QPushButton(this);
    gtasaButton->setMinimumSize(140, 100);
    gtasaButton->setMaximumSize(140, 100);
    gtasaButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    gtasaButton->setStyleSheet(
        "QPushButton {"
        "    border: 2px solid #FFD93D;"
        "    border-radius: 8px;"
        "    background-color: #2a2a2a;"
        "    padding: 10px;"
        "}"
        "QPushButton:hover {"
        "    border: 3px solid #FFD93D;"
        "    background-color: #3a3a3a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1a1a1a;"
        "}"
    );
    
    QString gtasaLogo = getLogoPath("gtasa.png");
    if (!gtasaLogo.isEmpty() && QFile::exists(gtasaLogo)) {
        QPixmap pixmap(gtasaLogo);
        QIcon icon(pixmap.scaled(120, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        gtasaButton->setIcon(icon);
        gtasaButton->setIconSize(QSize(120, 48));
    } else {
        gtasaButton->setText("GTA:SA");
        gtasaButton->setStyleSheet(gtasaButton->styleSheet() + "QPushButton { color: #FFD93D; font-size: 16px; font-weight: bold; }");
    }
    
    connect(gtasaButton, &QPushButton::clicked, this, &GameVersionDialog::onGTASASelected);
    buttonsLayout->addWidget(gtasaButton);
    
    mainLayout->addLayout(buttonsLayout);
    
    // Apply dark theme
    setStyleSheet(
        "QDialog {"
        "    background-color: #1a1a1a;"
        "    color: #e0e0e0;"
        "}"
    );
}

QString GameVersionDialog::getLogoPath(const QString& logoName) const {
    // Use Qt resource system
    QString resourcePath = ":/logos/" + logoName;
    if (QFile::exists(resourcePath)) {
        return resourcePath;
    }
    
    return QString();
}

void GameVersionDialog::onGTA3Selected() {
    selectedVersion = LibTXD::GameVersion::GTA3_4; // Use GTA3_4 as default
    accept();
}

void GameVersionDialog::onGTAVCSelected() {
    selectedVersion = LibTXD::GameVersion::VC_PC;
    accept();
}

void GameVersionDialog::onGTASASelected() {
    selectedVersion = LibTXD::GameVersion::SA;
    accept();
}

