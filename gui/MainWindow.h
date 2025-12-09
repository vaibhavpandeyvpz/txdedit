#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <memory>
#include "TXDModel.h"

class TexturePreviewWidget;
class TexturePropertiesWidget;
class TextureListWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void closeFile();
    void exit();
    void showAbout();
    
    void onTextureSelected(int index);
    void onTexturePropertyChanged();
    
    void addTexture();
    void removeTexture();
    void exportTexture();
    void importTexture();
    void bulkExport();
    
    void onExportRequested(int index);
    void onImportRequested(int index);
    void onReplaceDiffuseRequested(int index);
    void onReplaceAlphaRequested(int index);
    void onRemoveRequested(int index);
    void onPreviewTabChanged();

private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void applyStylesheet();
    void updateTextureList();
    void updateTexturePreview();
    void updateTextureProperties();
    void clearUI();
    void setStatusMessage(const QString& text);
    void updateGameVersionDisplay();
    void updateWindowTitle();
    
    bool loadTXD(const QString& filepath);
    bool saveTXD(const QString& filepath);
    QString getIconPath(const QString& iconName) const;
    
    TXDModel* model;
    int selectedTextureIndex;
    
    // UI Components
    QSplitter* mainSplitter;
    TextureListWidget* textureList;
    TexturePreviewWidget* previewWidget;
    TexturePropertiesWidget* propertiesWidget;
    QWidget* placeholderWidget;
    QPushButton* addBtn;
    QPushButton* removeBtn;
    
    // Status bar widgets
    QLabel* statusFileLabel;
    QLabel* statusTextureLabel;
    QLabel* statusSelectionLabel;
    QLabel* statusGameLabel;
    
    QAction* newAction = nullptr;
    QAction* openAction = nullptr;
    QAction* saveAction = nullptr;
    QAction* saveAsAction = nullptr;
    QAction* closeAction = nullptr;
    QAction* exitAction = nullptr;
    QAction* addTextureAction = nullptr;
    QAction* removeTextureAction = nullptr;
    QAction* exportTextureAction = nullptr;
    QAction* importTextureAction = nullptr;
    QAction* bulkExportAction = nullptr;
    QAction* toolbarSeparator = nullptr;
};

#endif // MAINWINDOW_H

