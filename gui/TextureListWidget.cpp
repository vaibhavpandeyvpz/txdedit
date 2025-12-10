#include "TextureListWidget.h"
#include "TXDModel.h"
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QPalette>
#include <QStyle>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>

TextureListWidget::TextureListWidget(QWidget *parent)
    : QListWidget(parent) {
    setViewMode(QListWidget::ListMode);
    setIconSize(QSize(32, 32));
    setSpacing(2);
    setItemDelegate(new TextureListItemDelegate(this));
}

QString TextureListWidget::formatTextureInfo(const TXDFileEntry* entry) const {
    if (!entry) {
        return "Invalid texture";
    }
    
    QString compressionStr = entry->compressionEnabled ? 
        (entry->hasAlpha ? "DXT3" : "DXT1") : "None";
    
    QString info = QString("Name: %1\nSize: %2x%3px\nHas alpha: %4\nCompression: %5")
        .arg(entry->name)
        .arg(entry->width)
        .arg(entry->height)
        .arg(entry->hasAlpha ? "Y" : "N")
        .arg(compressionStr);
    
    return info;
}

QPixmap TextureListWidget::createThumbnail(const uint8_t* rgbaData, int width, int height, bool hasAlpha) const {
    if (!rgbaData || width <= 0 || height <= 0) {
        return QPixmap();
    }
    
    // Create QImage directly from RGBA data
    QImage image(rgbaData, width, height, QImage::Format_RGBA8888);
    QImage imageCopy = image.copy();
    
    // If alpha is disabled, composite onto black background
    if (!hasAlpha) {
        QPixmap result(width, height);
        result.fill(Qt::black);
        QPainter painter(&result);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, imageCopy);
        painter.end();
        imageCopy = result.toImage();
    }
    
    // Create thumbnail (32x32 max)
    QPixmap pixmap = QPixmap::fromImage(imageCopy);
    if (pixmap.width() > 32 || pixmap.height() > 32) {
        pixmap = pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return pixmap;
}

void TextureListWidget::addTexture(const TXDFileEntry* entry, int index) {
    if (!entry) {
        return;
    }
    
    QString info = formatTextureInfo(entry);
    
    QListWidgetItem* item = new QListWidgetItem(info, this);
    
    // Create thumbnail from RGBA data
    if (!entry->diffuse.empty()) {
        QPixmap thumbnail = createThumbnail(entry->diffuse.data(), entry->width, entry->height, entry->hasAlpha);
    if (!thumbnail.isNull()) {
        item->setIcon(QIcon(thumbnail));
        }
    }
    
    // Store index as data
    item->setData(Qt::UserRole, index);
    
    // Set item height to accommodate multi-line text
    item->setSizeHint(QSize(item->sizeHint().width(), 80));
    
    addItem(item);
}

void TextureListWidget::updateTexture(const TXDFileEntry* entry, int index) {
    QListWidgetItem* item = nullptr;
    for (int i = 0; i < count(); i++) {
        QListWidgetItem* it = this->item(i);
        if (it && it->data(Qt::UserRole).toInt() == index) {
            item = it;
            break;
        }
    }
    
    if (item && entry) {
        QString info = formatTextureInfo(entry);
        item->setText(info);
        
        if (!entry->diffuse.empty()) {
            QPixmap thumbnail = createThumbnail(entry->diffuse.data(), entry->width, entry->height, entry->hasAlpha);
        if (!thumbnail.isNull()) {
            item->setIcon(QIcon(thumbnail));
            }
        }
    }
}

void TextureListWidget::clearTextures() {
    clear();
}

void TextureListWidget::contextMenuEvent(QContextMenuEvent* event) {
    QListWidgetItem* item = itemAt(event->pos());
    if (!item) {
        return;
    }
    
    int index = item->data(Qt::UserRole).toInt();
    
    QMenu menu(this);
    
    QAction* exportAction = menu.addAction("Export...");
    QAction* importAction = menu.addAction("Import...");
    menu.addSeparator();
    QAction* replaceDiffuseAction = menu.addAction("Replace diffuse...");
    QAction* replaceAlphaAction = menu.addAction("Replace alpha...");
    menu.addSeparator();
    QAction* removeAction = menu.addAction("Remove");
    
    QAction* selectedAction = menu.exec(event->globalPos());
    
    if (selectedAction == exportAction) {
        emit exportRequested(index);
    } else if (selectedAction == importAction) {
        emit importRequested(index);
    } else if (selectedAction == replaceDiffuseAction) {
        emit replaceDiffuseRequested(index);
    } else if (selectedAction == replaceAlphaAction) {
        emit replaceAlphaRequested(index);
    } else if (selectedAction == removeAction) {
        emit removeRequested(index);
    }
}
