#include "TexturePreviewWidget.h"
#include "TextureViewWidget.h"
#include "libtxd/txd_converter.h"
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QTimer>
#include <QSizePolicy>
#include <QTabBar>

TexturePreviewWidget::TexturePreviewWidget(QWidget *parent)
    : QWidget(parent), alphaTabIndex(-1), mixedTabIndex(-1), alphaTabsVisible(false) {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);
    
    // Create placeholder widget (shown when no texture)
    placeholderWidget = new QWidget(this);
    placeholderWidget->setStyleSheet("QWidget { background-color: #1a1a1a; }");
    mainLayout->addWidget(placeholderWidget);
    
    // Create tab widget (will be shown when texture is set)
    tabWidget = nullptr; // Create on demand
    imageView = nullptr;
    alphaView = nullptr;
    mixedView = nullptr;
    
    // Set dark background for the widget itself
    setStyleSheet("TexturePreviewWidget { background-color: #1a1a1a; }");
}

void TexturePreviewWidget::setTexture(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth, int originalHeight) {
    if (!texture || !data) {
        clear();
        return;
    }
    
    // Create tab widget if it doesn't exist
    if (!tabWidget) {
        tabWidget = new QTabWidget(this);
        
        // Image tab (always visible)
        imageView = new TextureViewWidget(this);
        tabWidget->addTab(imageView, "Image");
        
        // Alpha and Mixed tabs will be added/removed dynamically based on alpha channel
        alphaView = new TextureViewWidget(this);
        mixedView = new TextureViewWidget(this);
        
        // Connect to tab change signal to reset views when first shown
        connect(tabWidget, &QTabWidget::currentChanged, this, &TexturePreviewWidget::onTabChanged);
        
        mainLayout->addWidget(tabWidget);
    }
    
    // Show tab widget when texture is set
    placeholderWidget->hide();
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tabWidget->show();
    
    // Check if texture has alpha channel
    bool hasAlpha = texture->hasAlpha();
    
    // Add or remove alpha/mixed tabs based on alpha channel
    if (hasAlpha && !alphaTabsVisible) {
        // Add alpha and mixed tabs
        alphaTabIndex = tabWidget->addTab(alphaView, "Alpha / mask");
        mixedTabIndex = tabWidget->addTab(mixedView, "Combined");
        alphaTabsVisible = true;
    } else if (!hasAlpha && alphaTabsVisible) {
        // Remove alpha and mixed tabs
        tabWidget->removeTab(mixedTabIndex);
        tabWidget->removeTab(alphaTabIndex);
        alphaTabIndex = -1;
        mixedTabIndex = -1;
        alphaTabsVisible = false;
    }
    
    // Reset hasBeenShown flags so each view resets to 100% when first shown
    imageView->resetHasBeenShown();
    if (hasAlpha) {
        alphaView->resetHasBeenShown();
        mixedView->resetHasBeenShown();
    }
    
    // Use original dimensions if provided, otherwise use mipmap dimensions
    int origW = originalWidth;
    int origH = originalHeight;
    if (origW <= 0 || origH <= 0) {
        if (texture->getMipmapCount() > 0) {
            const auto& mipmap = texture->getMipmap(0);
            origW = mipmap.width;
            origH = mipmap.height;
        }
    }
    
    updateImageTab(texture, data, origW, origH);
    if (hasAlpha) {
        updateAlphaTab(texture, data, origW, origH);
        updateMixedTab(texture, data, origW, origH);
    }
    
    // Reset current tab to 100% immediately
    int currentTab = tabWidget->currentIndex();
    if (currentTab == 0) {
        imageView->zoom100();
    } else if (hasAlpha && currentTab == alphaTabIndex) {
        alphaView->zoom100();
    } else if (hasAlpha && currentTab == mixedTabIndex) {
        mixedView->zoom100();
    }
}

void TexturePreviewWidget::updateImageTab(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth, int originalHeight) {
    QPixmap pixmap = createImagePixmap(texture, data, false, false, originalWidth, originalHeight);
    imageView->setPixmap(pixmap);
}

void TexturePreviewWidget::updateAlphaTab(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth, int originalHeight) {
    QPixmap pixmap = createImagePixmap(texture, data, true, false, originalWidth, originalHeight);
    alphaView->setPixmap(pixmap);
}

void TexturePreviewWidget::updateMixedTab(const LibTXD::Texture* texture, const uint8_t* data, int originalWidth, int originalHeight) {
    QPixmap pixmap = createImagePixmap(texture, data, false, true, originalWidth, originalHeight);
    mixedView->setPixmap(pixmap);
}

QPixmap TexturePreviewWidget::createImagePixmap(const LibTXD::Texture* texture, const uint8_t* data, bool showAlpha, bool mixed, int targetWidth, int targetHeight) {
    if (!texture || !data) {
        return QPixmap();
    }
    
    // Convert texture to RGBA8
    auto rgbaData = LibTXD::TextureConverter::convertToRGBA8(*texture, 0);
    if (!rgbaData) {
        return QPixmap();
    }
    
    // Use dimensions from mipmap
    int dataWidth = targetWidth;
    int dataHeight = targetHeight;
    if (dataWidth <= 0 || dataHeight <= 0) {
        if (texture->getMipmapCount() > 0) {
            const auto& mipmap = texture->getMipmap(0);
            dataWidth = mipmap.width;
            dataHeight = mipmap.height;
        } else {
            return QPixmap();
        }
    }
    
    // Create QImage from RGBA data
    QImage image(rgbaData.get(), dataWidth, dataHeight, QImage::Format_RGBA8888);
    QImage imageCopy = image.copy();
    
    // Get final dimensions after scaling
    int finalWidth = imageCopy.width();
    int finalHeight = imageCopy.height();
    
    if (showAlpha) {
        // Show only alpha channel as grayscale
        for (int y = 0; y < finalHeight; y++) {
            for (int x = 0; x < finalWidth; x++) {
                QRgb pixel = imageCopy.pixel(x, y);
                uint8_t alpha = qAlpha(pixel);
                imageCopy.setPixel(x, y, qRgb(alpha, alpha, alpha));
            }
        }
    } else if (mixed) {
        // Show RGB with alpha as checkerboard pattern
        // Create checkerboard pattern
        QPixmap checkerPattern(16, 16);
        checkerPattern.fill(Qt::lightGray);
        QPainter checkerPainter(&checkerPattern);
        checkerPainter.fillRect(0, 0, 8, 8, Qt::white);
        checkerPainter.fillRect(8, 8, 8, 8, Qt::white);
        checkerPainter.fillRect(0, 8, 8, 8, Qt::lightGray);
        checkerPainter.fillRect(8, 0, 8, 8, Qt::lightGray);
        checkerPainter.end();
        
        // Create background image with checkerboard
        QImage mixedImage(finalWidth, finalHeight, QImage::Format_ARGB32);
        QPainter bgPainter(&mixedImage);
        QBrush checkerBrush(checkerPattern);
        bgPainter.fillRect(0, 0, finalWidth, finalHeight, checkerBrush);
        bgPainter.end();
        
        // Composite the texture over the checkerboard
        QPainter painter(&mixedImage);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, imageCopy);
        painter.end();
        
        imageCopy = mixedImage;
    }
    
    // Create pixmap
    QPixmap pixmap = QPixmap::fromImage(imageCopy);
    
    // Scale down if too large
    if (pixmap.width() > 512 || pixmap.height() > 512) {
        pixmap = pixmap.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return pixmap;
}

void TexturePreviewWidget::clear() {
    if (imageView) imageView->clear();
    if (alphaView) alphaView->clear();
    if (mixedView) mixedView->clear();
    
    // Remove alpha/mixed tabs if they exist
    if (tabWidget && alphaTabsVisible) {
        tabWidget->removeTab(mixedTabIndex);
        tabWidget->removeTab(alphaTabIndex);
        alphaTabIndex = -1;
        mixedTabIndex = -1;
        alphaTabsVisible = false;
    }
    
    // Hide tab widget when no texture
    if (tabWidget) {
        tabWidget->hide();
        // Remove from layout to prevent it from taking up space
        mainLayout->removeWidget(tabWidget);
    }
    // Show placeholder
    placeholderWidget->show();
}

void TexturePreviewWidget::onTabChanged(int index) {
    // Reset the current view to 100% when its tab is first shown
    TextureViewWidget* currentView = nullptr;
    if (index == 0) {
        currentView = imageView;
    } else if (alphaTabsVisible && index == alphaTabIndex) {
        currentView = alphaView;
    } else if (alphaTabsVisible && index == mixedTabIndex) {
        currentView = mixedView;
    }
    
    if (currentView && !currentView->hasBeenShownOnce()) {
        // Use QTimer to ensure widget is properly sized before zooming
        QTimer::singleShot(0, [currentView]() {
            currentView->zoom100();
            currentView->setHasBeenShownOnce(true);
        });
    }
}
