#ifndef TEXTUREPREVIEWWIDGET_H
#define TEXTUREPREVIEWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QHBoxLayout>
#include "libtxd/txd_texture.h"
#include "libtxd/txd_converter.h"
#include "TextureViewWidget.h"

class TexturePreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit TexturePreviewWidget(QWidget *parent = nullptr);
    // Simple: just pass RGBA data directly
    void setTexture(const uint8_t* rgbaData, int width, int height, bool hasAlpha);
    void clear();
    
    // Tab type for import functionality
    enum class ActiveTab { Image, Alpha, Mixed, None };
    ActiveTab getCurrentTab() const;

signals:
    void tabChanged(ActiveTab tab);

private slots:
    void onTabChanged(int index);

private:
    void updateImageTab(const uint8_t* rgbaData, int width, int height);
    void updateAlphaTab(const uint8_t* rgbaData, int width, int height);
    void updateMixedTab(const uint8_t* rgbaData, int width, int height);
    QPixmap createImagePixmap(const uint8_t* rgbaData, int width, int height, bool hasAlpha, bool showAlpha = false, bool mixed = false);
    
    QVBoxLayout* mainLayout;
    QTabWidget* tabWidget;
    QWidget* placeholderWidget;
    
    TextureViewWidget* imageView;
    TextureViewWidget* alphaView;
    TextureViewWidget* mixedView;
    
    int alphaTabIndex;
    int mixedTabIndex;
    bool alphaTabsVisible;
    bool currentHasAlpha;  // Store current alpha state
};

#endif // TEXTUREPREVIEWWIDGET_H

