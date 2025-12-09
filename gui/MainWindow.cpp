#include "MainWindow.h"
#include "TexturePreviewWidget.h"
#include "TexturePropertiesWidget.h"
#include "TextureListWidget.h"
#include "AboutDialog.h"
#include "GameVersionDialog.h"
#include "libtxd/txd_converter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QGroupBox>
#include <QFileInfo>
#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QPushButton>
#include <QAbstractButton>
#include <QPainter>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <cstring>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), model(new TXDModel(this)), selectedTextureIndex(-1) {
    setupMenus();  // Create actions first
    setupUI();     // Then setup UI which uses those actions
    
    // Connect model signals
    connect(model, &TXDModel::modelChanged, this, &MainWindow::updateTextureList);
    connect(model, &TXDModel::textureAdded, this, [this](size_t index) {
        if (!this || !propertiesWidget) return; // Guard against destruction
        updateTextureList();
        if (static_cast<int>(index) == selectedTextureIndex) {
            updateTexturePreview();
            updateTextureProperties();
        }
    });
    connect(model, &TXDModel::textureRemoved, this, [this](size_t index) {
        if (!this || !propertiesWidget) return; // Guard against destruction
        if (selectedTextureIndex >= static_cast<int>(index)) {
            if (selectedTextureIndex == static_cast<int>(index)) {
                selectedTextureIndex = -1;
                updateTexturePreview();
                updateTextureProperties();
            } else {
                selectedTextureIndex--;
            }
        }
        updateTextureList();
    });
    connect(model, &TXDModel::textureUpdated, this, [this](size_t index) {
        if (!this || !propertiesWidget) return; // Guard against destruction
        if (static_cast<int>(index) == selectedTextureIndex) {
            updateTexturePreview();
            updateTextureProperties();
        }
        updateTextureList();
    });
    connect(model, &TXDModel::modifiedChanged, this, [this](bool modified) {
        if (!this) return; // Guard against destruction
        updateWindowTitle();
    });
    
    clearUI();
}

MainWindow::~MainWindow() {
    // Disconnect signals to prevent handlers from firing during destruction
    if (model) {
        disconnect(model, nullptr, this, nullptr);
    }
    if (propertiesWidget) {
        disconnect(propertiesWidget, nullptr, this, nullptr);
    }
    // Clear propertiesWidget reference to prevent access during destruction
    propertiesWidget = nullptr;
}

QString MainWindow::getIconPath(const QString& iconName) const {
    // Use Qt resource system
    QString resourcePath = ":/icons/" + iconName;
    if (QFile::exists(resourcePath)) {
        return resourcePath;
    }
    return QString();
}

void MainWindow::setupUI() {
    setWindowTitle("TXD Edit by VPZ");
    setMinimumSize(800, 600);
    resize(1024, 600);
    
    // Set window icon if available
    QString iconPath;
#ifdef Q_OS_MACOS
    iconPath = getIconPath("mac.icns");
#elif defined(Q_OS_WIN)
    iconPath = getIconPath("windows.ico");
#else
    iconPath = getIconPath("mac.icns"); // Fallback
#endif
    if (!iconPath.isEmpty()) {
        setWindowIcon(QIcon(iconPath));
    }
    
    // Apply modern stylesheet
    applyStylesheet();
    
    // Create toolbar
    setupToolbar();
    
    // Create central widget with splitter
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(mainSplitter);
    
    // Left panel: Texture list (styled sidebar)
    QWidget* leftPanel = new QWidget;
    leftPanel->setObjectName("sidebar");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(8);
    
    // Placeholder for empty state
    placeholderWidget = new QWidget(leftPanel);
    placeholderWidget->setObjectName("placeholderWidget");
    QVBoxLayout* placeholderLayout = new QVBoxLayout(placeholderWidget);
    placeholderLayout->setAlignment(Qt::AlignCenter);
    QLabel* placeholderLabel = new QLabel("No textures", placeholderWidget);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setStyleSheet("QLabel { color: #888888; font-size: 14px; }");
    placeholderLayout->addWidget(placeholderLabel);
    placeholderWidget->setStyleSheet("#placeholderWidget { background-color: #1e1e1e; border: 1px solid #3a3a3a; }");
    
    textureList = new TextureListWidget(leftPanel);
    textureList->setObjectName("textureList");
    connect(textureList, &QListWidget::currentRowChanged, this, &MainWindow::onTextureSelected);
    connect(textureList, &TextureListWidget::exportRequested, this, &MainWindow::onExportRequested);
    connect(textureList, &TextureListWidget::importRequested, this, &MainWindow::onImportRequested);
    connect(textureList, &TextureListWidget::replaceDiffuseRequested, this, &MainWindow::onReplaceDiffuseRequested);
    connect(textureList, &TextureListWidget::replaceAlphaRequested, this, &MainWindow::onReplaceAlphaRequested);
    connect(textureList, &TextureListWidget::removeRequested, this, &MainWindow::onRemoveRequested);
    
    // Stack placeholder and list (only one visible at a time)
    leftLayout->addWidget(placeholderWidget);
    leftLayout->addWidget(textureList);
    
    // Initially show placeholder, hide list
    placeholderWidget->show();
    textureList->hide();
    
    // Set left panel size constraints
    leftPanel->setMinimumWidth(200);
    leftPanel->setMaximumWidth(300);
    leftPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding); // Preferred width, expanding height
    
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(5);
    addBtn = new QPushButton(QIcon(getIconPath("add.png")), "Add", leftPanel);
    addBtn->setObjectName("actionButton");
    addBtn->setIconSize(QSize(12, 12));
    removeBtn = new QPushButton(QIcon(getIconPath("remove.png")), "Remove", leftPanel);
    removeBtn->setIconSize(QSize(12, 12));
    // Remove button is less prominent - no special styling
    // Make buttons same size and fill horizontally 50-50
    addBtn->setFixedHeight(32);
    removeBtn->setFixedHeight(32);
    // Initially hidden (no file open, no texture selected)
    addBtn->setEnabled(false);
    addBtn->setVisible(false);
    removeBtn->setEnabled(false);
    removeBtn->setVisible(false);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addTexture);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::removeTexture);
    buttonLayout->addWidget(addBtn, 1); // Stretch factor 1
    buttonLayout->addWidget(removeBtn, 1); // Stretch factor 1
    leftLayout->addLayout(buttonLayout);
    
    mainSplitter->addWidget(leftPanel);
    
    // Center panel: Preview
    previewWidget = new TexturePreviewWidget(this);
    previewWidget->setObjectName("previewWidget");
    connect(previewWidget, &TexturePreviewWidget::tabChanged, this, &MainWindow::onPreviewTabChanged);
    mainSplitter->addWidget(previewWidget);
    
    // Right panel: Properties
    propertiesWidget = new TexturePropertiesWidget(this);
    propertiesWidget->setObjectName("propertiesWidget");
    connect(propertiesWidget, &TexturePropertiesWidget::propertyChanged, 
            this, &MainWindow::onTexturePropertyChanged);
    
    // Set right panel size constraints
    propertiesWidget->setMinimumWidth(300);
    propertiesWidget->setMaximumWidth(400);
    
    mainSplitter->addWidget(propertiesWidget);
    
    // Set splitter sizes: left 250, center takes rest, right 350
    mainSplitter->setSizes({250, 500, 350});
    mainSplitter->setStretchFactor(0, 0); // Left: fixed (no stretch)
    mainSplitter->setStretchFactor(1, 1);  // Center: stretchable
    mainSplitter->setStretchFactor(2, 0); // Right: fixed (no stretch)
    
    // Status bar with permanent widgets
    setupStatusBar();
}

void MainWindow::setupToolbar() {
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(14, 14)); // Set icon size for toolbar
    
    if (newAction) toolbar->addAction(newAction);
    if (openAction) toolbar->addAction(openAction);
    toolbarSeparator = toolbar->addSeparator();
    toolbarSeparator->setVisible(false); // Initially hidden (no file open)
    if (saveAction) toolbar->addAction(saveAction);
    if (saveAsAction) toolbar->addAction(saveAsAction);
    
    // Spacer to push import/export to the right
    QWidget* spacer = new QWidget(toolbar);
    spacer->setObjectName("toolbarSpacer");
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    
    // Right-aligned import/export actions
    if (exportTextureAction) toolbar->addAction(exportTextureAction);
    if (importTextureAction) toolbar->addAction(importTextureAction);
}

void MainWindow::setupStatusBar() {
    QStatusBar* bar = statusBar();
    bar->setSizeGripEnabled(false);
    
    statusFileLabel = new QLabel("File: None", this);
    statusTextureLabel = new QLabel("Textures: 0", this);
    statusGameLabel = new QLabel("", this);
    statusSelectionLabel = new QLabel("Ready", this);
    
    bar->addWidget(statusFileLabel);
    bar->addWidget(statusTextureLabel);
    bar->addWidget(statusGameLabel);
    bar->addPermanentWidget(statusSelectionLabel, 1);
}

void MainWindow::setStatusMessage(const QString& text) {
    if (statusSelectionLabel) {
        statusSelectionLabel->setText(text);
    } else {
        statusBar()->showMessage(text);
    }
}

void MainWindow::applyStylesheet() {
    QString style = R"(
        /* Global dark theme - apply to all widgets */
        * {
            background-color: #1a1a1a;
            color: #e0e0e0;
        }
        
        /* Main window - Dark GTA theme */
        QMainWindow {
            background-color: #1a1a1a;
            color: #e0e0e0;
        }
        
        /* Sidebar */
        #sidebar {
            background-color: #252525;
            border-right: 2px solid #3a3a3a;
        }
        
        /* Texture list */
        #textureList {
            border: 1px solid #3a3a3a;
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        
        #textureList::item {
            padding: 5px;
            border-bottom: 1px solid #2a2a2a;
            color: #e0e0e0;
        }
        
        #textureList::item:selected {
            background-color: #ff8800;
            color: #ffffff;
            border: 1px solid #ffaa00;
        }
        
        #textureList::item:selected:hover {
            background-color: #ffaa00; /* Lighter orange */
            color: #ffffff;
            border: 1px solid #ffaa00;
        }
        
        #textureList::item:hover {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        /* Custom scrollbars - Dark theme */
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 14px;
            border: 1px solid #3a3a3a;
            margin: 0;
        }
        
        QScrollBar::handle:vertical {
            background-color: #4a4a4a;
            min-height: 30px;
            margin: 2px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #5a5a5a;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background-color: #1e1e1e;
            height: 14px;
            border: 1px solid #3a3a3a;
            margin: 0;
        }
        
        QScrollBar::handle:horizontal {
            background-color: #4a4a4a;
            min-width: 30px;
            margin: 2px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: #5a5a5a;
        }
        
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        
        /* Buttons - GTA style */
        #actionButton {
            background-color: #ff6600;
            color: #ffffff;
            border: 2px solid #ff8800;
            padding: 6px 12px;
            font-weight: bold;
        }
        
        #actionButton:hover {
            background-color: #ff8800;
            border: 2px solid #ffaa00;
        }
        
        #actionButton:pressed {
            background-color: #cc5500;
        }
        
        #actionButton:disabled {
            background-color: #1a1a1a;
            color: #555555;
            border: 2px solid #2a2a2a;
        }
        
        QPushButton {
            background-color: #2a2a2a;
            border: 1px solid #4a4a4a;
            padding: 6px 12px;
            color: #e0e0e0;
        }
        
        QPushButton:hover {
            background-color: #3a3a3a;
            border: 1px solid #5a5a5a;
        }
        
        QPushButton:pressed {
            background-color: #1a1a1a;
        }
        
        QPushButton:disabled {
            background-color: #1a1a1a;
            color: #555555;
            border: 1px solid #2a2a2a;
        }
        
        /* Toolbar - Dark theme */
        QToolBar {
            background-color: #252525;
            border-bottom: 2px solid #3a3a3a;
            spacing: 5px;
            padding: 8px;
        }
        
        QToolBar::separator {
            background-color: #3a3a3a;
            width: 1px;
            margin: 4px 2px;
        }
        
        QToolBar #toolbarSpacer {
            background-color: #252525;
        }
        
        QToolBar QToolButton {
            background-color: #2a2a2a;
            border: 1px solid #4a4a4a;
            padding: 6px 12px;
            color: #e0e0e0;
        }
        
        QToolBar QToolButton:hover {
            background-color: #3a3a3a;
            border: 1px solid #ff8800;
            color: #ffffff;
        }
        
        QToolBar QToolButton:pressed {
            background-color: #1a1a1a;
        }
        
        /* Group boxes - Dark theme */
        QGroupBox {
            font-weight: bold;
            border: 2px solid #3a3a3a;
            margin-top: 12px;
            padding-top: 12px;
            background-color: #1e1e1e;
            color: #ff8800;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 8px;
            color: #ff8800;
        }
        
        /* Line edits - Dark theme */
        QLineEdit {
            border: 1px solid #4a4a4a;
            padding: 6px;
            background-color: #2a2a2a;
            color: #e0e0e0;
        }
        
        QLineEdit:focus {
            border: 2px solid #00d4ff;
            background-color: #2d2d2d;
        }
        
        /* Spin boxes - Dark theme */
        QSpinBox {
            border: 1px solid #4a4a4a;
            padding: 5px;
            background-color: #2a2a2a;
            color: #e0e0e0;
        }
        
        QSpinBox:focus {
            border: 2px solid #00d4ff;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #3a3a3a;
            border: 1px solid #5a5a5a;
            width: 20px;
        }
        
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #4a4a4a;
        }
        
        /* Combo boxes - Dark theme */
        QComboBox {
            border: 1px solid #4a4a4a;
            padding: 6px;
            background-color: #2a2a2a;
            color: #e0e0e0;
        }
        
        QComboBox:hover {
            border: 1px solid #ff8800;
            background-color: #2d2d2d;
        }
        
        QComboBox:focus {
            border: 2px solid #ff8800;
            background-color: #2d2d2d;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 0px;
            background-color: transparent;
        }
        
        QComboBox::drop-down:hover {
            background-color: transparent;
        }
        
        QComboBox QAbstractItemView {
            background-color: #1a1a1a;
            border: 1px solid #3a3a3a;
            color: #e0e0e0;
            selection-background-color: #ff8800;
            selection-color: #ffffff;
            outline: none;
            padding: 0px;
            margin: 0px;
        }
        
        QComboBox QAbstractItemView::item {
            padding: 6px 8px;
            border: none;
            min-height: 20px;
        }
        
        QComboBox QAbstractItemView::item:hover {
            background-color: #3a3a3a;
            color: #ff8800;
        }
        
        QComboBox QAbstractItemView::item:selected {
            background-color: #ff8800;
            color: #ffffff;
        }
        
        QComboBox QAbstractItemView::item:selected:hover {
            background-color: #ffaa00;
            color: #ffffff;
        }
        
        QListView {
            background-color: #1a1a1a;
            border: 1px solid #3a3a3a;
            padding: 0px;
            margin: 0px;
        }
        
        QListView::viewport {
            background-color: #1a1a1a;
            border: none;
        }
        
        QListView::item {
            background-color: transparent;
            padding: 6px 8px;
            border: none;
            margin: 0px;
            min-height: 20px;
        }
        
        QListView::item:hover {
            background-color: #3a3a3a;
            color: #ff8800;
        }
        
        QListView::item:selected {
            background-color: #ff8800;
            color: #ffffff;
        }
        
        QListView::item:selected:hover {
            background-color: #ffaa00;
            color: #ffffff;
        }
        
        /* Context menu - Dark theme */
        QMenu {
            background-color: #1a1a1a;
            border: 1px solid #3a3a3a;
            color: #e0e0e0;
            padding: 4px;
        }
        
        QMenu::item {
            background-color: transparent;
            padding: 6px 24px 6px 8px;
            border: none;
            min-height: 20px;
        }
        
        QMenu::item:hover {
            background-color: #3a3a3a;
            color: #ff8800;
        }
        
        QMenu::item:selected {
            background-color: #ff8800;
            color: #ffffff;
        }
        
        QMenu::separator {
            height: 1px;
            background-color: #3a3a3a;
            margin: 4px 0px;
        }
        
        /* Fix dropdown viewport borders */
        QComboBox::view {
            background-color: #1a1a1a;
            border: 1px solid #3a3a3a;
            padding: 0px;
            margin: 0px;
        }
        
        QComboBox QAbstractItemView::viewport {
            background-color: #1a1a1a;
            border: none;
        }
        
        /* Check boxes - Dark theme */
        QCheckBox {
            spacing: 8px;
            color: #e0e0e0;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #4a4a4a;
            background-color: #2a2a2a;
        }
        
        QCheckBox::indicator:hover {
            border: 2px solid #ff8800;
        }
        
        QCheckBox::indicator:checked {
            background-color: #ff8800;
            border: 2px solid #ff8800;
        }
        
        /* Sliders - GTA style */
        QSlider::groove:horizontal {
            height: 8px;
            background: #2a2a2a;
            border: 1px solid #4a4a4a;
        }
        
        QSlider::handle:horizontal {
            background: #ff6600;
            border: 2px solid #ff8800;
            width: 20px;
            height: 20px;
            margin: -6px 0;
        }
        
        QSlider::handle:horizontal:hover {
            background: #ff8800;
            border: 2px solid #ffaa00;
        }
        
        /* Tab widget - Dark theme */
        QTabWidget::pane {
            border: 1px solid #3a3a3a;
            background-color: #1a1a1a;
        }
        
        QTabBar::tab {
            background-color: #252525;
            color: #888888;
            padding: 10px 20px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            margin-right: 2px;
            border: 1px solid #3a3a3a;
        }
        
        QTabBar::tab:selected {
            background-color: #1a1a1a;
            color: #ff8800;
            border-bottom: 3px solid #ff8800;
            border-top: 1px solid #3a3a3a;
            border-left: 1px solid #3a3a3a;
            border-right: 1px solid #3a3a3a;
        }
        
        QTabBar::tab:hover {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        /* Labels */
        QLabel {
            color: #e0e0e0;
        }
        
        /* Form layout labels */
        QFormLayout QLabel {
            color: #b0b0b0;
        }
        
        /* Status bar */
        QStatusBar {
            background-color: #252525;
            color: #e0e0e0;
            border-top: 2px solid #3a3a3a;
            padding: 4px 8px;
            min-height: 26px;
        }
        
        QStatusBar QLabel {
            color: #b0b0b0;
            background-color: transparent;
            margin: 0 10px;
        }
        
        QStatusBar::item {
            border: none;
            background-color: transparent;
        }
        
        /* Splitter */
        QSplitter::handle {
            background-color: #3a3a3a;
        }
        
        QSplitter::handle:hover {
            background-color: #4a4a4a;
        }
        
        /* Graphics view background */
        QGraphicsView {
            background-color: #1a1a1a;
            border: 1px solid #3a3a3a;
        }
        
        /* Preview widget background */
        #previewWidget {
            background-color: #1a1a1a;
        }
        
        /* Hide tab widget completely when not in use */
        QTabWidget {
            background-color: #1a1a1a;
        }
        
        QTabWidget::pane {
            background-color: #1a1a1a;
        }
        
        QTabBar {
            background-color: #1a1a1a;
        }
        
        /* Properties widget background */
        #propertiesWidget {
            background-color: #252525;
        }
        
        /* Scroll area background */
        QScrollArea {
            background-color: #252525;
            border: none;
        }
        
        QScrollArea QWidget {
            background-color: #252525;
        }
        
        /* Dialog boxes - Dark theme */
        QMessageBox {
            background-color: #1a1a1a;
            color: #e0e0e0;
        }
        
        QMessageBox QLabel {
            background-color: #1a1a1a;
            color: #e0e0e0;
        }
        
        QMessageBox QPushButton {
            background-color: #2a2a2a;
            border: 1px solid #4a4a4a;
            padding: 6px 12px;
            color: #e0e0e0;
            min-width: 80px;
        }
        
        QMessageBox QPushButton:hover {
            background-color: #3a3a3a;
            border: 1px solid #5a5a5a;
        }
        
        QMessageBox QPushButton:pressed {
            background-color: #1a1a1a;
        }
        
        /* File dialog - Dark theme */
        QFileDialog {
            background-color: #1a1a1a;
            color: #e0e0e0;
        }
        
        QFileDialog QLabel {
            background-color: #1a1a1a;
            color: #e0e0e0;
        }
        
        QFileDialog QTreeView, QFileDialog QListView {
            background-color: #1a1a1a;
            color: #e0e0e0;
            border: 1px solid #3a3a3a;
        }
        
        QFileDialog QTreeView::item, QFileDialog QListView::item {
            color: #e0e0e0;
            padding: 4px;
        }
        
        QFileDialog QTreeView::item:selected, QFileDialog QListView::item:selected {
            background-color: #0066cc;
            color: #ffffff;
        }
        
        QFileDialog QTreeView::item:hover, QFileDialog QListView::item:hover {
            background-color: #2d2d2d;
        }
        
        QFileDialog QLineEdit {
            background-color: #2a2a2a;
            border: 1px solid #4a4a4a;
            color: #e0e0e0;
            padding: 6px;
        }
        
        QFileDialog QComboBox {
            background-color: #2a2a2a;
            border: 1px solid #4a4a4a;
            color: #e0e0e0;
        }
        
        QFileDialog QComboBox::drop-down {
            border: none;
            width: 0px;
            background-color: transparent;
        }
        
        QFileDialog QComboBox QAbstractItemView {
            background-color: #1a1a1a;
            border: 1px solid #3a3a3a;
            selection-background-color: #ff8800;
            selection-color: #ffffff;
        }
    )";
    
    setStyleSheet(style);
}

void MainWindow::setupMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    // Create menu actions without icons (macOS menu bar doesn't respect setIconVisibleInMenu)
    QAction* newMenuAction = fileMenu->addAction("&New", QKeySequence::New);
    connect(newMenuAction, &QAction::triggered, this, &MainWindow::newFile);
    // Create toolbar action with icon (reuse same slot)
    newAction = new QAction(QIcon(getIconPath("new-file.png")), "New", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    
    fileMenu->addSeparator();
    
    QAction* openMenuAction = fileMenu->addAction("&Open...", QKeySequence::Open);
    connect(openMenuAction, &QAction::triggered, this, &MainWindow::openFile);
    // Create toolbar action with icon (reuse same slot)
    openAction = new QAction(QIcon(getIconPath("open.png")), "Open", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addSeparator();
    saveAction = fileMenu->addAction("&Save", QKeySequence::Save);
    saveAction->setIcon(QIcon(getIconPath("save.png")));
    saveAction->setIconVisibleInMenu(false); // Hide icon in menu, show in toolbar
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    saveAsAction = fileMenu->addAction("Save &as...", QKeySequence::SaveAs);
    saveAsAction->setIcon(QIcon(getIconPath("save-as.png")));
    saveAsAction->setIconVisibleInMenu(false); // Hide icon in menu, show in toolbar
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAsFile);
    fileMenu->addSeparator();
    closeAction = fileMenu->addAction("&Close");
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeFile);
    fileMenu->addSeparator();
    exitAction = fileMenu->addAction("E&xit", QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::exit);
    
    // Texture menu
    QMenu* textureMenu = menuBar()->addMenu("&Texture");
    addTextureAction = textureMenu->addAction("&Add texture...");
    addTextureAction->setIcon(QIcon(getIconPath("add.png")));
    addTextureAction->setIconVisibleInMenu(false); // Hide icon in menu, show in toolbar/buttons
    connect(addTextureAction, &QAction::triggered, this, &MainWindow::addTexture);
    removeTextureAction = textureMenu->addAction("&Remove texture");
    removeTextureAction->setIcon(QIcon(getIconPath("remove.png")));
    removeTextureAction->setIconVisibleInMenu(false); // Hide icon in menu, show in toolbar/buttons
    connect(removeTextureAction, &QAction::triggered, this, &MainWindow::removeTexture);
    textureMenu->addSeparator();
    exportTextureAction = textureMenu->addAction("&Export");
    exportTextureAction->setIcon(QIcon(getIconPath("export.png")));
    exportTextureAction->setIconVisibleInMenu(false); // Hide icon in menu, show in toolbar
    connect(exportTextureAction, &QAction::triggered, this, &MainWindow::exportTexture);
    importTextureAction = textureMenu->addAction("&Import");
    importTextureAction->setIcon(QIcon(getIconPath("import.png")));
    importTextureAction->setIconVisibleInMenu(false); // Hide icon in menu, show in toolbar
    connect(importTextureAction, &QAction::triggered, this, &MainWindow::importTexture);
    bulkExportAction = textureMenu->addAction("&Bulk export...");
    connect(bulkExportAction, &QAction::triggered, this, &MainWindow::bulkExport);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    // Update action states
    saveAction->setEnabled(false);
    saveAction->setVisible(false);
    saveAsAction->setEnabled(false);
    saveAsAction->setVisible(false);
    if (toolbarSeparator) toolbarSeparator->setVisible(false);
    closeAction->setEnabled(false);
    addTextureAction->setEnabled(false);
    removeTextureAction->setEnabled(false);
    exportTextureAction->setEnabled(false);
    importTextureAction->setEnabled(false);
    bulkExportAction->setEnabled(false);
    // Import/export are also hidden until a texture is selected
    exportTextureAction->setVisible(false);
    importTextureAction->setVisible(false);
    
}

void MainWindow::newFile() {
    
    // Check if there's a current file open
    if (model->getTextureCount() > 0 || !model->getFilePath().isEmpty()) {
        int ret = QMessageBox::question(this, "New File",
            "Creating a new file will close the current file. Do you want to save the current file first?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Cancel) {
            return; // User cancelled
        } else if (ret == QMessageBox::Save) {
            // Try to save
            if (!model->getFilePath().isEmpty()) {
                saveFile();
            } else {
                saveAsFile();
            }
            // If save was cancelled, don't create new file
            if (model->getTextureCount() > 0 && !model->getFilePath().isEmpty()) {
                return;
            }
        }
    }
    
    // Ask user for game version
    GameVersionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return; // User cancelled
    }
    
    LibTXD::GameVersion selectedVersion = dialog.getSelectedVersion();
    if (selectedVersion == LibTXD::GameVersion::UNKNOWN) {
        return; // Invalid selection
    }
    
    // Create new empty TXD model
    model->clear();
    // Set version based on game version
    uint32_t version = 0x1803FFFF; // Default to SA
    if (selectedVersion == LibTXD::GameVersion::GTA3_1 || selectedVersion == LibTXD::GameVersion::GTA3_4) {
        version = 0x0800FFFF;
    } else if (selectedVersion == LibTXD::GameVersion::VC_PC) {
        version = 0x1003FFFF;
    }
    // Note: version will be set when saving, for now just clear
    model->setFilePath(QString());
    
    // Update UI
    clearUI();
    setWindowTitle("TXD Edit by VPZ");
    if (statusFileLabel) {
        statusFileLabel->setText("File: None");
    }
    if (statusTextureLabel) {
        statusTextureLabel->setText("Textures: 0");
    }
    updateGameVersionDisplay();
    setStatusMessage("New file created");
    
    // Enable file operations
    saveAction->setEnabled(true);
    saveAction->setVisible(true);
    saveAsAction->setEnabled(true);
    saveAsAction->setVisible(true);
    if (toolbarSeparator) toolbarSeparator->setVisible(true);
    closeAction->setEnabled(true);
    addTextureAction->setEnabled(true);
    removeTextureAction->setEnabled(true);
    bulkExportAction->setEnabled(true);
    
    // Show add button
    if (addBtn) {
        addBtn->setEnabled(true);
        addBtn->setVisible(true);
    }
}

void MainWindow::openFile() {
    // Ensure window is active on macOS
    raise();
    activateWindow();
    
    // Use native file dialog on macOS
    QString filepath = QFileDialog::getOpenFileName(
        this, 
        "Open TXD File", 
        QDir::homePath(), 
        "TXD Files (*.txd);;All Files (*)",
        nullptr,
        QFileDialog::DontUseNativeDialog
    );
    
    if (!filepath.isEmpty()) {
        if (loadTXD(filepath)) {
            model->setFilePath(filepath);
            updateWindowTitle();
            if (statusFileLabel) {
                statusFileLabel->setText("File: " + QFileInfo(filepath).fileName());
            }
            if (statusTextureLabel) {
                statusTextureLabel->setText(QString("Textures: %1").arg(model->getTextureCount()));
            }
            updateGameVersionDisplay();
            setStatusMessage("Path: " + filepath);
            
            saveAction->setEnabled(true);
            saveAction->setVisible(true);
            saveAsAction->setEnabled(true);
            saveAsAction->setVisible(true);
            if (toolbarSeparator) toolbarSeparator->setVisible(true);
            closeAction->setEnabled(true);
            addTextureAction->setEnabled(true);
            removeTextureAction->setEnabled(true);
            bulkExportAction->setEnabled(true);
            // Show add/remove buttons when file is open
            if (addBtn) {
                addBtn->setEnabled(true);
                addBtn->setVisible(true);
            }
            // Import/export remain hidden until a texture is selected
        }
    }
}

void MainWindow::saveFile() {
    if (model->getTextureCount() == 0 && model->getFilePath().isEmpty()) {
        QMessageBox::warning(this, "No File", "No file is currently open.");
        return;
    }
    
    QString filepath = model->getFilePath();
    if (filepath.isEmpty()) {
        saveAsFile();
    } else {
        if (saveTXD(filepath)) {
            model->setModified(false);
            setStatusMessage("File saved successfully");
        }
    }
}

void MainWindow::saveAsFile() {
    QString filepath = QFileDialog::getSaveFileName(
        this, "Save TXD File", model->getFilePath(), "TXD Files (*.txd);;All Files (*)"
    );
    
    if (!filepath.isEmpty()) {
        if (saveTXD(filepath)) {
            model->setFilePath(filepath);
            model->setModified(false);
            updateWindowTitle();
            if (statusFileLabel) {
                statusFileLabel->setText("File: " + QFileInfo(filepath).fileName());
            }
            setStatusMessage("File saved: " + filepath);
        }
    }
}

void MainWindow::closeFile() {
    if (model->getTextureCount() > 0 || !model->getFilePath().isEmpty()) {
        model->clear();
        clearUI();
        updateWindowTitle();
        if (statusFileLabel) {
            statusFileLabel->setText("File: None");
        }
        if (statusTextureLabel) {
            statusTextureLabel->setText("Textures: 0");
        }
        if (statusGameLabel) {
            statusGameLabel->setText("");
        }
        setStatusMessage("File closed");
        
        saveAction->setEnabled(false);
        saveAction->setVisible(false);
        saveAsAction->setEnabled(false);
        saveAsAction->setVisible(false);
        if (toolbarSeparator) toolbarSeparator->setVisible(false);
        closeAction->setEnabled(false);
        addTextureAction->setEnabled(false);
        removeTextureAction->setEnabled(false);
        exportTextureAction->setEnabled(false);
        importTextureAction->setEnabled(false);
        bulkExportAction->setEnabled(false);
        exportTextureAction->setVisible(false);
        importTextureAction->setVisible(false);
        // Hide add/remove buttons when file is closed
        if (addBtn) {
            addBtn->setEnabled(false);
            addBtn->setVisible(false);
        }
        if (removeBtn) {
            removeBtn->setEnabled(false);
            removeBtn->setVisible(false);
        }
    }
}

void MainWindow::exit() {
    close();
}

void MainWindow::showAbout() {
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::updateGameVersionDisplay() {
    if (!statusGameLabel || model->getTextureCount() == 0) {
        if (statusGameLabel) {
            statusGameLabel->setText("");
        }
        return;
    }
    
    LibTXD::GameVersion gameVersion = model->getGameVersion();
    QString gameName;
    QString color;
    
    switch (gameVersion) {
        case LibTXD::GameVersion::GTA3_1:
        case LibTXD::GameVersion::GTA3_2:
        case LibTXD::GameVersion::GTA3_3:
        case LibTXD::GameVersion::GTA3_4:
            gameName = "GTA:III";
            color = "#00a8ff";
            break;
        case LibTXD::GameVersion::VC_PC:
        case LibTXD::GameVersion::VC_PS2:
            gameName = "GTA:VC";
            color = "#f195ac";
            break;
        case LibTXD::GameVersion::SA:
            gameName = "GTA:SA";
            color = "#906210";
            break;
        default:
            gameName = "Unknown";
            color = "#e1e1e1";
            break;
    }
    
    statusGameLabel->setText(QString("<span style='color: %1; font-weight: bold;'>%2</span>").arg(color, gameName));
}

void MainWindow::updateWindowTitle() {
    QString title = "TXD Edit by VPZ";
    QString filepath = model->getFilePath();
    if (!filepath.isEmpty()) {
        title += " - " + QFileInfo(filepath).fileName();
    }
    if (model->isModified()) {
        title += " *";
    }
    setWindowTitle(title);
}

bool MainWindow::loadTXD(const QString& filepath) {
    if (!model->loadFromFile(filepath)) {
        QMessageBox::critical(this, "Error", 
            QString("Failed to load TXD file:\n%1").arg(filepath));
        return false;
    }
    updateTextureList();
    if (statusTextureLabel) {
        statusTextureLabel->setText(QString("Textures: %1").arg(model->getTextureCount()));
    }
    updateGameVersionDisplay();
    setStatusMessage(QString("Loaded %1 textures").arg(model->getTextureCount()));
    return true;
}

bool MainWindow::saveTXD(const QString& filepath) {
    if (model->getTextureCount() == 0) {
        return false;
    }
    
    if (!model->saveToFile(filepath)) {
        QMessageBox::critical(this, "Error", 
            QString("Failed to save TXD file:\n%1").arg(filepath));
        setStatusMessage("Save failed");
        return false;
    }
    
    setStatusMessage("File saved successfully");
    return true;
}

void MainWindow::updateTextureList() {
    // Preserve current selection before clearing
    int currentRow = textureList->currentRow();
    int preservedIndex = -1;
    if (currentRow >= 0) {
        QListWidgetItem* item = textureList->item(currentRow);
        if (item) {
            preservedIndex = item->data(Qt::UserRole).toInt();
        }
    }
    
    textureList->clearTextures();
    
    if (!model || model->getTextureCount() == 0) {
        // Show placeholder, hide list
        if (placeholderWidget) placeholderWidget->show();
        textureList->hide();
        selectedTextureIndex = -1;
        // Clear preview and properties when no textures
        previewWidget->clear();
        if (propertiesWidget) propertiesWidget->clear();
        // Update button states and status bar
        if (addBtn) addBtn->setEnabled(model != nullptr); // Enable if file is open
        if (removeBtn) removeBtn->setEnabled(false); // Disable if no texture selected
        if (statusTextureLabel) {
            statusTextureLabel->setText("Textures: 0");
        }
        return;
    }
    
    // Hide placeholder, show list
    if (placeholderWidget) placeholderWidget->hide();
    textureList->show();
    
    for (size_t i = 0; i < model->getTextureCount(); i++) {
        textureList->addTexture(model->getTexture(i), static_cast<int>(i));
    }
    
    // Restore selection if it was valid, otherwise select first texture
    if (preservedIndex >= 0 && preservedIndex < static_cast<int>(model->getTextureCount())) {
        // Find the item with the preserved index
        for (int i = 0; i < textureList->count(); i++) {
            QListWidgetItem* item = textureList->item(i);
            if (item && item->data(Qt::UserRole).toInt() == preservedIndex) {
                textureList->setCurrentRow(i);
                selectedTextureIndex = preservedIndex;
                break;
            }
        }
    } else if (textureList->count() > 0) {
        // Auto-select first texture if no preserved selection
        // onTextureSelected will be called automatically via currentRowChanged signal
        textureList->setCurrentRow(0);
    }
    
    // Update button states and status bar
    if (addBtn) addBtn->setEnabled(model != nullptr); // Enable if file is open
    if (statusTextureLabel) {
        statusTextureLabel->setText(QString("Textures: %1").arg(model->getTextureCount()));
    }
    // removeBtn state will be updated by onTextureSelected
}

void MainWindow::onTextureSelected(int index) {
    if (index < 0) {
        // No texture selected - clear preview and properties content
        selectedTextureIndex = -1;
        previewWidget->clear();
        if (propertiesWidget) propertiesWidget->clear();
        // Disable and hide remove button
        if (removeBtn) {
            removeBtn->setEnabled(false);
            removeBtn->setVisible(false);
        }
        if (exportTextureAction) {
            exportTextureAction->setEnabled(false);
            exportTextureAction->setVisible(false);
        }
        if (importTextureAction) {
            importTextureAction->setEnabled(false);
            importTextureAction->setVisible(false);
        }
        return;
    }
    
    // Get the actual texture index from the item data
    QListWidgetItem* item = textureList->item(index);
    if (item) {
        selectedTextureIndex = item->data(Qt::UserRole).toInt();
    } else {
        selectedTextureIndex = index;
    }
    
    // Update preview and properties when texture is selected
    updateTexturePreview();
    updateTextureProperties();
    
    // Enable and show remove button when texture is selected
    if (removeBtn) {
        removeBtn->setEnabled(true);
        removeBtn->setVisible(true);
    }
    
    // Show and enable import/export when a texture is selected
    if (exportTextureAction) {
        exportTextureAction->setEnabled(true);
        exportTextureAction->setVisible(true);
    }
    if (importTextureAction) {
        importTextureAction->setEnabled(true);
        importTextureAction->setVisible(true);
    }
}

void MainWindow::updateTexturePreview() {
    if (!model || selectedTextureIndex < 0) {
        previewWidget->clear();
        return;
    }
    
    TXDFileEntry* entry = model->getTexture(selectedTextureIndex);
    if (!entry || entry->diffuse.empty()) {
        previewWidget->clear();
        return;
    }
    
    // Use RGBA data directly
    previewWidget->setTexture(entry->diffuse.data(), entry->width, entry->height, entry->hasAlpha);
}

void MainWindow::updateTextureProperties() {
    if (!propertiesWidget) {
        return; // Widget is being destroyed
    }
    
    if (!model || selectedTextureIndex < 0) {
        if (propertiesWidget) propertiesWidget->clear();
        if (propertiesWidget) propertiesWidget->hide();
        return;
    }
    
    TXDFileEntry* entry = model->getTexture(selectedTextureIndex);
    if (entry) {
        if (propertiesWidget) {
            propertiesWidget->show();
            propertiesWidget->setTexture(entry);
        }
    } else {
        if (propertiesWidget) propertiesWidget->clear();
        if (propertiesWidget) propertiesWidget->hide();
    }
}

void MainWindow::onTexturePropertyChanged() {
    if (!model || selectedTextureIndex < 0) {
        return;
    }
    
    // Properties are updated directly on the texture entry in the model
    // Properties are updated directly on the texture entry
    model->setModified(true);
    updateTexturePreview();
    updateTextureList(); // In case name changed
}

void MainWindow::clearUI() {
    if (statusGameLabel) {
        statusGameLabel->setText("");
    }
    textureList->clear();
    previewWidget->clear();
    propertiesWidget->clear();
    selectedTextureIndex = -1;
    
    // Show placeholder when UI is cleared
    if (placeholderWidget) placeholderWidget->show();
    textureList->hide();
    
    // Disable buttons when no file is open
    if (addBtn) addBtn->setEnabled(false);
    if (removeBtn) removeBtn->setEnabled(false);
}

void MainWindow::addTexture() {
    if (!model) {
        QMessageBox::warning(this, "No File", "Please open or create a TXD file first.");
        return;
    }
    
    // Ensure window is active on macOS
    raise();
    activateWindow();
    
    // Ask for image file - try non-native dialog
    QString filepath = QFileDialog::getOpenFileName(
        this, 
        "Add Texture", 
        QDir::homePath(),
        "Image Files (*.png *.jpg *.jpeg *.bmp);;PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)",
        nullptr,
        QFileDialog::DontUseNativeDialog
    );
    
    if (filepath.isEmpty()) {
        return;
    }
    
    // Load image
    QImage image(filepath);
    if (image.isNull()) {
        QMessageBox::critical(this, "Add Error", "Failed to load image file.");
        return;
    }
    
    // Convert to RGBA8888 if needed
    QImage rgbaImage = image.convertedTo(QImage::Format_RGBA8888);
    
    // Get texture name from filename
    QFileInfo fileInfo(filepath);
    QString textureName = fileInfo.baseName();
    
    // Check if texture with this name already exists
    TXDFileEntry* existing = model->findTexture(textureName);
    if (existing) {
        int ret = QMessageBox::question(this, "Texture Exists",
            QString("A texture named '%1' already exists. Replace it?").arg(textureName),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
        // Remove existing texture
        model->removeTexture(textureName);
    }
    
    // Create texture entry
    uint32_t width = rgbaImage.width();
    uint32_t height = rgbaImage.height();
    bool hasAlpha = rgbaImage.hasAlphaChannel();
    
    // Ensure dimensions are valid
    if (width < 1 || width > 4096 || height < 1 || height > 4096) {
        QMessageBox::critical(this, "Add Error", 
            QString("Invalid image dimensions: %1x%2. Must be between 1x1 and 4096x4096.").arg(width).arg(height));
        return;
    }
    
    TXDFileEntry entry;
    entry.name = textureName;
    entry.maskName = QString();
    entry.rasterFormat = hasAlpha ? LibTXD::RasterFormat::B8G8R8A8 : LibTXD::RasterFormat::B8G8R8;
    entry.compressionEnabled = false; // Compression off by default
    entry.width = width;
    entry.height = height;
    entry.hasAlpha = hasAlpha;
    entry.mipmapCount = 1;
    entry.filterFlags = 0;
    entry.isNew = true;
    
    // Copy RGBA data
    const uint8_t* imageData = rgbaImage.constBits();
    size_t dataSize = width * height * 4;
    entry.diffuse.assign(imageData, imageData + dataSize);
    
    // Add to model
    model->addTexture(std::move(entry));
    model->setModified(true);
    
    // Update UI
    updateTextureList();
    setStatusMessage(QString("Added texture: %1").arg(textureName));
}

void MainWindow::removeTexture() {
    if (!model || selectedTextureIndex < 0) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Remove Texture", 
        "Are you sure you want to remove this texture?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        model->removeTexture(selectedTextureIndex);
        model->setModified(true);
        selectedTextureIndex = -1;
        if (propertiesWidget) propertiesWidget->clear();
        if (propertiesWidget) propertiesWidget->hide();
        // Disable remove button after removal
        if (removeBtn) removeBtn->setEnabled(false);
        updateTextureList();
        setStatusMessage("Texture removed");
    }
}

void MainWindow::exportTexture() {
    if (!model || selectedTextureIndex < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a texture to export.");
        return;
    }
    
    TXDFileEntry* entry = model->getTexture(selectedTextureIndex);
    if (!entry || entry->diffuse.empty()) {
        QMessageBox::warning(this, "Export Error", "Texture has no data.");
        return;
    }
    
    // Create QImage directly from RGBA data
    QImage rgbaImage(entry->diffuse.data(), entry->width, entry->height, QImage::Format_RGBA8888);
    QImage image = rgbaImage.copy(); // Make a copy
    bool hasAlpha = entry->hasAlpha;
    
    // Determine export type
    enum ExportType { DiffuseOnly, AlphaOnly, Both };
    ExportType exportType = DiffuseOnly;
    
    if (hasAlpha) {
        // Ask user which type to export
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Export Texture");
        msgBox.setText("What would you like to export?");
        msgBox.setIcon(QMessageBox::Question);
        
        QPushButton* diffuseBtn = msgBox.addButton("Diffuse only", QMessageBox::ActionRole);
        QPushButton* alphaBtn = msgBox.addButton("Alpha only", QMessageBox::ActionRole);
        QPushButton* bothBtn = msgBox.addButton("Both", QMessageBox::ActionRole);
        QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
        
        msgBox.exec();
        
        QAbstractButton* clicked = msgBox.clickedButton();
        if (clicked == cancelBtn) {
            return; // User cancelled
        } else if (clicked == diffuseBtn) {
            exportType = DiffuseOnly;
        } else if (clicked == alphaBtn) {
            exportType = AlphaOnly;
        } else if (clicked == bothBtn) {
            exportType = Both;
        } else {
            return; // Should not happen
        }
    }
    
    // Get base filename
    QString baseName = entry->name;
    if (baseName.isEmpty()) {
        baseName = "texture";
    }
    
    // Export based on type
    if (exportType == DiffuseOnly) {
        // Export diffuse only
        QString suggestedName = baseName + ".png";
        QString filepath = QFileDialog::getSaveFileName(
            this, "Export Diffuse Texture", suggestedName,
            "PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)"
        );
        
        if (!filepath.isEmpty()) {
            if (rgbaImage.save(filepath)) {
                setStatusMessage("Diffuse texture exported: " + filepath);
            } else {
                QMessageBox::critical(this, "Export Error", "Failed to save image file.");
                setStatusMessage("Texture export failed");
            }
        }
    } else if (exportType == AlphaOnly) {
        // Export alpha channel as grayscale
        QImage alphaImage = rgbaImage.copy();
        int width = alphaImage.width();
        int height = alphaImage.height();
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                QRgb pixel = alphaImage.pixel(x, y);
                uint8_t alpha = qAlpha(pixel);
                alphaImage.setPixel(x, y, qRgb(alpha, alpha, alpha));
            }
        }
        
        QString suggestedName = baseName + "_alpha.png";
        QString filepath = QFileDialog::getSaveFileName(
            this, "Export Alpha Channel", suggestedName,
            "PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)"
        );
        
        if (!filepath.isEmpty()) {
            if (alphaImage.save(filepath)) {
                setStatusMessage("Alpha channel exported: " + filepath);
            } else {
                QMessageBox::critical(this, "Export Error", "Failed to save image file.");
                setStatusMessage("Texture export failed");
            }
        }
    } else if (exportType == Both) {
        // Export both diffuse and alpha
        QString suggestedName = baseName + ".png";
        QString filepath = QFileDialog::getSaveFileName(
            this, "Export Diffuse Texture", suggestedName,
            "PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)"
        );
        
        if (!filepath.isEmpty()) {
            // Save diffuse
            if (rgbaImage.save(filepath)) {
                // Save alpha with _alpha suffix
                QFileInfo fileInfo(filepath);
                QString alphaPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + "_alpha." + fileInfo.suffix();
                
                QImage alphaImage = rgbaImage.copy();
                int width = alphaImage.width();
                int height = alphaImage.height();
                
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        QRgb pixel = alphaImage.pixel(x, y);
                        uint8_t alpha = qAlpha(pixel);
                        alphaImage.setPixel(x, y, qRgb(alpha, alpha, alpha));
                    }
                }
                
                if (alphaImage.save(alphaPath)) {
                    setStatusMessage(QString("Exported diffuse and alpha: %1, %2").arg(filepath, alphaPath));
                } else {
                    setStatusMessage("Diffuse exported, but alpha export failed: " + filepath);
                    QMessageBox::warning(this, "Export Warning", 
                        QString("Diffuse texture saved, but failed to save alpha channel:\n%1").arg(alphaPath));
                }
            } else {
                QMessageBox::critical(this, "Export Error", "Failed to save diffuse image file.");
                setStatusMessage("Texture export failed");
            }
        }
    }
}

void MainWindow::importTexture() {
    if (!model) {
        QMessageBox::warning(this, "No File", "Please open a TXD file first.");
        return;
    }
    
    // Check if a texture is selected and which tab is active
    auto activeTab = previewWidget->getCurrentTab();
    
    // If texture is selected and on Image or Alpha tab, replace that data
    if (selectedTextureIndex >= 0) {
        if (activeTab == TexturePreviewWidget::ActiveTab::Image) {
            // Replace diffuse
            onReplaceDiffuseRequested(selectedTextureIndex);
            return;
        } else if (activeTab == TexturePreviewWidget::ActiveTab::Alpha) {
            // Replace alpha
            onReplaceAlphaRequested(selectedTextureIndex);
            return;
        } else if (activeTab == TexturePreviewWidget::ActiveTab::Mixed) {
            // Mixed tab - import should be disabled, but just in case
            QMessageBox::warning(this, "Import Error", 
                "Cannot import on Combined view. Switch to Image or Alpha tab.");
            return;
        }
    }
    
    // No texture selected or tab not applicable - add new texture
    QString filepath = QFileDialog::getOpenFileName(
        this, "Import Texture", "",
        "Image Files (*.png *.jpg *.jpeg *.bmp);;PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)"
    );
    
    if (filepath.isEmpty()) {
        return;
    }
    
    // Load image
    QImage image(filepath);
    if (image.isNull()) {
        QMessageBox::critical(this, "Import Error", "Failed to load image file.");
        return;
    }
    
    // Convert to RGBA8888 if needed
    QImage rgbaImage = image.convertedTo(QImage::Format_RGBA8888);
    
    // Get texture name from filename
    QFileInfo fileInfo(filepath);
    QString textureName = fileInfo.baseName();
    
    // Check if texture with this name already exists
    TXDFileEntry* existing = model->findTexture(textureName);
    if (existing) {
        int ret = QMessageBox::question(this, "Texture Exists",
            QString("A texture named '%1' already exists. Replace it?").arg(textureName),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
        // Remove existing texture
        model->removeTexture(textureName);
    }
    
    // Create texture entry in model
    uint32_t width = rgbaImage.width();
    uint32_t height = rgbaImage.height();
    bool hasAlpha = rgbaImage.hasAlphaChannel();
    
    TXDFileEntry entry;
    entry.name = textureName;
    entry.maskName = QString();
    entry.rasterFormat = hasAlpha ? LibTXD::RasterFormat::B8G8R8A8 : LibTXD::RasterFormat::B8G8R8;
    entry.compressionEnabled = false; // Compression off by default
    entry.width = width;
    entry.height = height;
    entry.hasAlpha = hasAlpha;
    entry.mipmapCount = 1;
    entry.filterFlags = 0;
    entry.isNew = true;
    
    // Copy RGBA data
    const uint8_t* imageData = rgbaImage.constBits();
    size_t dataSize = width * height * 4;
    entry.diffuse.assign(imageData, imageData + dataSize);
    
    // Add to model
    model->addTexture(std::move(entry));
    model->setModified(true);
    
    // Update UI
    updateTextureList();
    setStatusMessage(QString("Imported texture: %1").arg(textureName));
}

void MainWindow::bulkExport() {
    if (!model || model->getTextureCount() == 0) {
        QMessageBox::warning(this, "No File", "Please open a TXD file first.");
        return;
    }
    
    size_t textureCount = model->getTextureCount();
    if (textureCount == 0) {
        QMessageBox::warning(this, "No Textures", "The current TXD file has no textures to export.");
        return;
    }
    
    // Ask for export folder
    QString folderPath = QFileDialog::getExistingDirectory(
        this, "Select Export Folder", "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (folderPath.isEmpty()) {
        return; // User cancelled
    }
    
    // Ensure folder path ends with separator
    if (!folderPath.endsWith('/') && !folderPath.endsWith('\\')) {
        folderPath += '/';
    }
    
    int successCount = 0;
    int failCount = 0;
    int alphaCount = 0;
    
    // Export all textures
    for (size_t i = 0; i < textureCount; i++) {
        TXDFileEntry* entry = model->getTexture(i);
        if (!entry) {
            failCount++;
            continue;
        }
        
        // Use preview pixmap from model
        // Create QImage directly from RGBA data
        if (entry->diffuse.empty()) {
            failCount++;
            continue;
        }
        
        QImage rgbaImage(entry->diffuse.data(), entry->width, entry->height, QImage::Format_RGBA8888);
        QImage image = rgbaImage.copy(); // Make a copy
        
        // Get base filename
        QString baseName = entry->name;
        if (baseName.isEmpty()) {
            baseName = QString("texture_%1").arg(i);
        }
        
        // Export diffuse
        QString diffusePath = folderPath + baseName + ".png";
        if (image.save(diffusePath)) {
            successCount++;
        } else {
            failCount++;
            continue; // Skip alpha if diffuse failed
        }
        
        // Export alpha if texture has alpha channel
        bool hasAlpha = entry->hasAlpha;
        if (hasAlpha) {
            QImage alphaImage = rgbaImage.copy();
            int width = alphaImage.width();
            int height = alphaImage.height();
            
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    QRgb pixel = alphaImage.pixel(x, y);
                    uint8_t alpha = qAlpha(pixel);
                    alphaImage.setPixel(x, y, qRgb(alpha, alpha, alpha));
                }
            }
            
            QString alphaPath = folderPath + baseName + "_alpha.png";
            if (alphaImage.save(alphaPath)) {
                alphaCount++;
            }
            // Don't count alpha failure as overall failure
        }
    }
    
    // Show summary
    QString message = QString("Bulk export completed:\n\n"
                              "Successfully exported: %1 texture(s)\n"
                              "Alpha channels exported: %2\n"
                              "Failed: %3 texture(s)")
                      .arg(successCount).arg(alphaCount).arg(failCount);
    
    if (failCount > 0) {
        QMessageBox::warning(this, "Bulk Export", message);
    } else {
        QMessageBox::information(this, "Bulk Export", message);
    }
    
    setStatusMessage(QString("Bulk exported %1 texture(s) to %2").arg(successCount).arg(folderPath));
}

void MainWindow::onExportRequested(int index) {
    // Temporarily set selectedTextureIndex to the requested index
    int oldIndex = selectedTextureIndex;
    selectedTextureIndex = index;
    
    // Select the item in the list to match
    for (int i = 0; i < textureList->count(); i++) {
        QListWidgetItem* item = textureList->item(i);
        if (item && item->data(Qt::UserRole).toInt() == index) {
            textureList->setCurrentRow(i);
            break;
        }
    }
    
    // Call export function
    exportTexture();
    
    // Restore old selection
    selectedTextureIndex = oldIndex;
}

void MainWindow::onImportRequested(int index) {
    // Import replaces diffuse or alpha based on active tab
    importTexture();
}

void MainWindow::onPreviewTabChanged() {
    // Enable/disable import based on active tab
    if (!importTextureAction) {
        return;
    }
    
    // If no texture selected, import is already disabled
    if (selectedTextureIndex < 0) {
        return;
    }
    
    auto tab = previewWidget->getCurrentTab();
    
    // Disable import on mixed/combined tab
    if (tab == TexturePreviewWidget::ActiveTab::Mixed) {
        importTextureAction->setEnabled(false);
    } else if (tab == TexturePreviewWidget::ActiveTab::Image || 
               tab == TexturePreviewWidget::ActiveTab::Alpha) {
        importTextureAction->setEnabled(true);
    }
}

void MainWindow::onReplaceDiffuseRequested(int index) {
    if (!model || index < 0 || index >= static_cast<int>(model->getTextureCount())) {
        QMessageBox::warning(this, "Invalid Index", "Invalid texture index.");
        return;
    }
    
    TXDFileEntry* entry = model->getTexture(index);
    if (!entry) {
        QMessageBox::warning(this, "Error", "Failed to get texture entry.");
        return;
    }
    
    // Ask for image file
    QString filepath = QFileDialog::getOpenFileName(
        this, "Replace Diffuse Image", "",
        "Image Files (*.png *.jpg *.jpeg *.bmp);;PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)"
    );
    
    if (filepath.isEmpty()) {
        return;
    }
    
    // Load image
    QImage image(filepath);
    if (image.isNull()) {
        QMessageBox::critical(this, "Import Error", "Failed to load image file.");
        return;
    }
    
    // Convert to RGBA8888 if needed
    QImage rgbaImage = image.convertedTo(QImage::Format_RGBA8888);
    
    uint32_t oldWidth = entry->width;
    uint32_t oldHeight = entry->height;
    bool hadAlpha = entry->hasAlpha;
    
    // Store original imported image dimensions (before any resizing)
    int importedWidth = rgbaImage.width();
    int importedHeight = rgbaImage.height();
    
    // Check dimensions match
    if (rgbaImage.width() != static_cast<int>(oldWidth) || rgbaImage.height() != static_cast<int>(oldHeight)) {
        int ret = QMessageBox::question(this, "Dimension Mismatch",
            QString("The image dimensions (%1x%2) don't match the texture dimensions (%3x%4).\n"
                    "Resize the image to match?").arg(rgbaImage.width()).arg(rgbaImage.height())
                    .arg(oldWidth).arg(oldHeight),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            rgbaImage = rgbaImage.scaled(oldWidth, oldHeight, 
                                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            return;
        }
    }
    
    // Check if new diffuse resolution differs from old diffuse resolution
    // If the imported image had different dimensions than the old texture, and texture had alpha,
    // we need to reset alpha to white (even if user chose to resize to match)
    // The requirement: "if diffuse is imported and alpha resolution differs from that of new diffuse"
    // This means: if the imported diffuse resolution differs from the old alpha resolution
    bool importedDimensionsDiffer = (importedWidth != static_cast<int>(oldWidth) || 
                                     importedHeight != static_cast<int>(oldHeight));
    bool needsAlphaReset = hadAlpha && importedDimensionsDiffer;
    
    // Check if dimensions changed after optional resize
    bool dimensionsChanged = (rgbaImage.width() != static_cast<int>(oldWidth) || 
                              rgbaImage.height() != static_cast<int>(oldHeight));
    
    // Get existing RGBA data to preserve alpha if needed
    std::unique_ptr<uint8_t[]> existingRGBA;
    if (!entry->diffuse.empty() && entry->diffuse.size() == oldWidth * oldHeight * 4) {
        // Copy existing RGBA data
        size_t dataSize = oldWidth * oldHeight * 4;
        existingRGBA = std::make_unique<uint8_t[]>(dataSize);
        std::memcpy(existingRGBA.get(), entry->diffuse.data(), dataSize);
    }
    
    // Prepare new texture data
    const uint8_t* imageData = rgbaImage.constBits();
    uint32_t newWidth = rgbaImage.width();
    uint32_t newHeight = rgbaImage.height();
    size_t dataSize = newWidth * newHeight * 4;
    std::vector<uint8_t> newTextureData(dataSize);
    
    if (hadAlpha && existingRGBA && !dimensionsChanged) {
        // Preserve existing alpha channel, replace RGB (dimensions match)
        for (size_t i = 0; i < dataSize; i += 4) {
            newTextureData[i] = imageData[i];     // R
            newTextureData[i + 1] = imageData[i + 1]; // G
            newTextureData[i + 2] = imageData[i + 2]; // B
            newTextureData[i + 3] = existingRGBA[i + 3]; // A (preserve existing)
        }
        entry->hasAlpha = true;
    } else {
        // Replace everything (including alpha if new image has it)
        std::memcpy(newTextureData.data(), imageData, dataSize);
        
        // If dimensions changed and texture had alpha, reset alpha to white (#ffffff)
        if (needsAlphaReset) {
            for (size_t i = 3; i < dataSize; i += 4) {
                newTextureData[i] = 255; // White alpha
            }
            entry->hasAlpha = true; // Keep alpha enabled
        } else {
            // Update alpha flag based on new image
            bool newHasAlpha = rgbaImage.hasAlphaChannel();
            entry->hasAlpha = newHasAlpha;
        }
    }
    
    // Update entry data
    entry->diffuse = newTextureData;
    entry->width = newWidth;
    entry->height = newHeight;
    model->setModified(true);
    
    // Update UI
    updateTextureList();
    if (selectedTextureIndex == index) {
        updateTexturePreview();
        updateTextureProperties();
    }
    
    setStatusMessage(QString("Replaced diffuse image for texture: %1").arg(entry->name));
}

void MainWindow::onReplaceAlphaRequested(int index) {
    if (!model || index < 0 || index >= static_cast<int>(model->getTextureCount())) {
        QMessageBox::warning(this, "Invalid Index", "Invalid texture index.");
        return;
    }
    
    TXDFileEntry* entry = model->getTexture(index);
    if (!entry) {
        QMessageBox::warning(this, "Error", "Failed to get texture entry.");
        return;
    }
    
    // Ask for image file (alpha channel)
    QString filepath = QFileDialog::getOpenFileName(
        this, "Replace Alpha Channel", "",
        "Image Files (*.png *.jpg *.jpeg *.bmp);;PNG Images (*.png);;JPEG Images (*.jpg *.jpeg);;BMP Images (*.bmp);;All Files (*)"
    );
    
    if (filepath.isEmpty()) {
        return;
    }
    
    // Load image
    QImage image(filepath);
    if (image.isNull()) {
        QMessageBox::critical(this, "Import Error", "Failed to load image file.");
        return;
    }
    
    // Convert to RGBA8888 if needed
    QImage rgbaImage = image.convertedTo(QImage::Format_RGBA8888);
    
    uint32_t width = entry->width;
    uint32_t height = entry->height;
    
    // Check dimensions match - if they don't, alert user and cancel (no resize option)
    if (rgbaImage.width() != static_cast<int>(width) || rgbaImage.height() != static_cast<int>(height)) {
        QMessageBox::warning(this, "Dimension Mismatch",
            QString("The alpha image dimensions (%1x%2) don't match the diffuse texture dimensions (%3x%4).\n\n"
                   "Alpha channel resolution must match the diffuse resolution.\n"
                   "Operation cancelled.").arg(rgbaImage.width()).arg(rgbaImage.height())
                   .arg(width).arg(height));
        return;
    }
    
    // Get existing texture data to preserve RGB
    if (entry->diffuse.empty() || entry->diffuse.size() != width * height * 4) {
        QMessageBox::warning(this, "Error", "Failed to get existing texture data.");
        return;
    }
    
    // Copy existing RGBA data
    size_t dataSize = width * height * 4;
    std::unique_ptr<uint8_t[]> existingRGBA = std::make_unique<uint8_t[]>(dataSize);
    std::memcpy(existingRGBA.get(), entry->diffuse.data(), dataSize);
    
    // Prepare new texture data
    std::vector<uint8_t> newTextureData(dataSize);
    
    const uint8_t* imageData = rgbaImage.constBits();
    
    // Preserve RGB, replace alpha from the new image
    // Use the grayscale value of the new image as alpha
    for (int y = 0; y < static_cast<int>(height); y++) {
        for (int x = 0; x < static_cast<int>(width); x++) {
            size_t pixelIndex = (y * width + x) * 4;
            
            // Preserve RGB from existing texture
            newTextureData[pixelIndex] = existingRGBA[pixelIndex];         // R
            newTextureData[pixelIndex + 1] = existingRGBA[pixelIndex + 1]; // G
            newTextureData[pixelIndex + 2] = existingRGBA[pixelIndex + 2]; // B
            
            // Use grayscale value from new image as alpha
            QRgb pixel = rgbaImage.pixel(x, y);
            uint8_t alpha = qAlpha(pixel);
            if (alpha == 255 && rgbaImage.hasAlphaChannel()) {
                // Image has alpha channel but this pixel is fully opaque, use grayscale
                newTextureData[pixelIndex + 3] = qGray(pixel);
            } else if (alpha < 255) {
                // Use the alpha channel value
                newTextureData[pixelIndex + 3] = alpha;
            } else {
                // No alpha channel, use grayscale
                newTextureData[pixelIndex + 3] = qGray(pixel);
            }
        }
    }
    
    // Update entry data
    entry->diffuse = newTextureData;
    entry->hasAlpha = true;
    model->setModified(true);
    
    // Update UI
    updateTextureList();
    if (selectedTextureIndex == index) {
        updateTexturePreview();
        updateTextureProperties();
    }
    
    setStatusMessage(QString("Replaced alpha channel for texture: %1").arg(entry->name));
}

void MainWindow::onRemoveRequested(int index) {
    // Temporarily set selectedTextureIndex to the requested index
    int oldIndex = selectedTextureIndex;
    selectedTextureIndex = index;
    
    // Select the item in the list to match
    for (int i = 0; i < textureList->count(); i++) {
        QListWidgetItem* item = textureList->item(i);
        if (item && item->data(Qt::UserRole).toInt() == index) {
            textureList->setCurrentRow(i);
            break;
        }
    }
    
    // Call remove function
    removeTexture();
    
    // Restore old selection (though it may be invalid after removal)
    selectedTextureIndex = oldIndex;
}

