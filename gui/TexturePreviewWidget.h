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
    void setTexture(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth = 0, int originalHeight = 0);
    void clear();

private slots:
    void onTabChanged(int index);

private:
    void updateImageTab(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth = 0, int originalHeight = 0);
    void updateAlphaTab(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth = 0, int originalHeight = 0);
    void updateMixedTab(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth = 0, int originalHeight = 0);
    QPixmap createImagePixmap(const LibTXD::Texture* texture, const uint8_t* data, bool showAlpha = false, bool mixed = false, int originalWidth = 0, int originalHeight = 0);
    
    QVBoxLayout* mainLayout;
    QTabWidget* tabWidget;
    QWidget* placeholderWidget;
    
    TextureViewWidget* imageView;
    TextureViewWidget* alphaView;
    TextureViewWidget* mixedView;
    
    int alphaTabIndex;
    int mixedTabIndex;
    bool alphaTabsVisible;
};

#endif // TEXTUREPREVIEWWIDGET_H

