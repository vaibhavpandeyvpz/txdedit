#include "TexturePreviewWidget.h"
#include <QPainter>
#include <QPixmap>
#include <QImage>

TexturePreviewWidget::TexturePreviewWidget(QWidget *parent)
    : QWidget(parent)
    , mainLayout(nullptr)
    , tabWidget(nullptr)
    , placeholderWidget(nullptr)
    , imageView(nullptr)
    , alphaView(nullptr)
    , mixedView(nullptr)
    , alphaTabIndex(-1)
    , mixedTabIndex(-1)
    , alphaTabsVisible(false)
    , currentHasAlpha(false)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Placeholder for empty state
    placeholderWidget = new QWidget(this);
    QVBoxLayout* placeholderLayout = new QVBoxLayout(placeholderWidget);
    placeholderLayout->setAlignment(Qt::AlignCenter);
    QLabel* placeholderLabel = new QLabel("No texture selected", placeholderWidget);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setStyleSheet("QLabel { color: #888888; font-size: 14px; }");
    placeholderLayout->addWidget(placeholderLabel);
    
    mainLayout->addWidget(placeholderWidget);
    placeholderWidget->show();
    
    setStyleSheet("TexturePreviewWidget { background-color: #1a1a1a; }");
}

void TexturePreviewWidget::setTexture(const uint8_t* rgbaData, int width, int height, bool hasAlpha) {
    if (!rgbaData || width <= 0 || height <= 0) {
        clear();
        return;
    }
    
    // Create tab widget if it doesn't exist
    if (!tabWidget) {
        tabWidget = new QTabWidget(this);
        
        // Image tab (always visible)
        imageView = new TextureViewWidget(this);
        tabWidget->addTab(imageView, "Image");
        
        // Alpha and Mixed tabs will be added/removed dynamically
        alphaView = new TextureViewWidget(this);
        mixedView = new TextureViewWidget(this);
        
        // Connect to tab change signal
        connect(tabWidget, &QTabWidget::currentChanged, this, &TexturePreviewWidget::onTabChanged);
        
        mainLayout->addWidget(tabWidget);
    }
    
    // Show tab widget
    placeholderWidget->hide();
    tabWidget->show();
    
    // Store alpha state
    currentHasAlpha = hasAlpha;
    
    // Add or remove alpha/mixed tabs based on alpha channel
    if (hasAlpha && !alphaTabsVisible) {
        alphaTabIndex = tabWidget->addTab(alphaView, "Alpha / mask");
        mixedTabIndex = tabWidget->addTab(mixedView, "Combined");
        alphaTabsVisible = true;
    } else if (!hasAlpha && alphaTabsVisible) {
        tabWidget->removeTab(mixedTabIndex);
        tabWidget->removeTab(alphaTabIndex);
        alphaTabIndex = -1;
        mixedTabIndex = -1;
        alphaTabsVisible = false;
    }
    
    // Reset views
    imageView->resetHasBeenShown();
    if (hasAlpha) {
        alphaView->resetHasBeenShown();
        mixedView->resetHasBeenShown();
    }
    
    // Update tabs
    updateImageTab(rgbaData, width, height);
    if (hasAlpha) {
        updateAlphaTab(rgbaData, width, height);
        updateMixedTab(rgbaData, width, height);
    }
    
    // Reset current tab to 100%
    int currentTab = tabWidget->currentIndex();
    if (currentTab == 0) {
        imageView->zoom100();
    } else if (hasAlpha && currentTab == alphaTabIndex) {
        alphaView->zoom100();
    } else if (hasAlpha && currentTab == mixedTabIndex) {
        mixedView->zoom100();
    }
}

void TexturePreviewWidget::updateImageTab(const uint8_t* rgbaData, int width, int height) {
    QPixmap pixmap = createImagePixmap(rgbaData, width, height, currentHasAlpha, false, false);
    imageView->setPixmap(pixmap);
}

void TexturePreviewWidget::updateAlphaTab(const uint8_t* rgbaData, int width, int height) {
    QPixmap pixmap = createImagePixmap(rgbaData, width, height, currentHasAlpha, true, false);
    alphaView->setPixmap(pixmap);
}

void TexturePreviewWidget::updateMixedTab(const uint8_t* rgbaData, int width, int height) {
    QPixmap pixmap = createImagePixmap(rgbaData, width, height, currentHasAlpha, false, true);
    mixedView->setPixmap(pixmap);
}

QPixmap TexturePreviewWidget::createImagePixmap(const uint8_t* rgbaData, int width, int height, bool hasAlpha, bool showAlpha, bool mixed) {
    if (!rgbaData || width <= 0 || height <= 0) {
        return QPixmap();
    }
    
    // Create QImage directly from RGBA data
    QImage image(rgbaData, width, height, QImage::Format_RGBA8888);
    QImage imageCopy = image.copy(); // Make a copy
    
    if (showAlpha) {
        // Show only alpha channel as grayscale
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                QRgb pixel = imageCopy.pixel(x, y);
                uint8_t alpha = qAlpha(pixel);
                imageCopy.setPixel(x, y, qRgb(alpha, alpha, alpha));
            }
        }
    } else if (mixed) {
        // Show RGB with alpha as checkerboard pattern
        QPixmap checkerPattern(16, 16);
        checkerPattern.fill(Qt::lightGray);
        QPainter checkerPainter(&checkerPattern);
        checkerPainter.fillRect(0, 0, 8, 8, Qt::white);
        checkerPainter.fillRect(8, 8, 8, 8, Qt::white);
        checkerPainter.end();
        
        QPixmap result = QPixmap::fromImage(imageCopy);
        QPainter painter(&result);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        painter.fillRect(result.rect(), QBrush(checkerPattern));
        painter.end();
        
        return result;
    } else if (!hasAlpha) {
        // Alpha is disabled - composite onto black background
        QPixmap result(width, height);
        result.fill(Qt::black);
        QPainter painter(&result);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, imageCopy);
        painter.end();
        return result;
    }
    
    return QPixmap::fromImage(imageCopy);
}

void TexturePreviewWidget::clear() {
    if (tabWidget) {
        tabWidget->hide();
    }
    if (placeholderWidget) {
        placeholderWidget->show();
    }
    if (imageView) {
        imageView->clear();
    }
    if (alphaView) {
        alphaView->clear();
    }
    if (mixedView) {
        mixedView->clear();
    }
}

TexturePreviewWidget::ActiveTab TexturePreviewWidget::getCurrentTab() const {
    if (!tabWidget || !tabWidget->isVisible()) {
        return ActiveTab::None;
    }
    
    int currentIndex = tabWidget->currentIndex();
    if (currentIndex == 0) {
        return ActiveTab::Image;
    } else if (alphaTabsVisible && currentIndex == alphaTabIndex) {
        return ActiveTab::Alpha;
    } else if (alphaTabsVisible && currentIndex == mixedTabIndex) {
        return ActiveTab::Mixed;
    }
    return ActiveTab::None;
}

void TexturePreviewWidget::onTabChanged(int index) {
    // Reset zoom when switching tabs
    if (index == 0 && imageView) {
        imageView->zoom100();
    } else if (alphaTabsVisible && index == alphaTabIndex && alphaView) {
        alphaView->zoom100();
    } else if (alphaTabsVisible && index == mixedTabIndex && mixedView) {
        mixedView->zoom100();
    }
    
    // Emit tab changed signal
    emit tabChanged(getCurrentTab());
}
