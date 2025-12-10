#ifndef TEXTUREPROPERTIESWIDGET_H
#define TEXTUREPROPERTIESWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QListView>
#include "CheckBox.h"
#include "TXDModel.h"

class TexturePropertiesWidget : public QWidget {
    Q_OBJECT

public:
    explicit TexturePropertiesWidget(QWidget *parent = nullptr);
    void setTexture(TXDFileEntry* entry);
    void clear();

signals:
    void propertyChanged();

private slots:
    void onNameChanged();
    void onAlphaNameChanged();
    void onAlphaChannelToggled(bool enabled);
    void onCompressionToggled(bool enabled);

private:
    void updateUI();
    void blockSignals(bool block);
    
    TXDFileEntry* currentEntry;
    
    QScrollArea* scrollArea;
    QWidget* contentWidget;
    
    QGroupBox* propertiesGroup;
    QLineEdit* nameEdit;
    QLineEdit* alphaNameEdit;
    QLabel* widthLabel;
    QLabel* heightLabel;
    QLabel* mipmapLabel;
    CheckBox* alphaCheck;
    QLabel* formatLabel;
    QComboBox* formatCombo;  // Hidden, kept for compatibility
    CheckBox* compressionCheck;
    
    QGroupBox* flagsGroup;
    QComboBox* filterCombo;
    QComboBox* uWrapCombo;
    QComboBox* vWrapCombo;
};

#endif // TEXTUREPROPERTIESWIDGET_H

