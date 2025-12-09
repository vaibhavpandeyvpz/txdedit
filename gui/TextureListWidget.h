#ifndef TEXTURELISTWIDGET_H
#define TEXTURELISTWIDGET_H

#include <QListWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QStyle>
#include <QApplication>
#include "libtxd/txd_texture.h"
#include "libtxd/txd_converter.h"

// Custom delegate to align icons to top
class TextureListItemDelegate : public QStyledItemDelegate {
public:
    explicit TextureListItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        
        // Let the style draw the background (for hover/selected states)
        QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
        
        // Draw icon aligned to top
        QRect iconRect = opt.rect;
        iconRect.setLeft(opt.rect.left() + 5);
        iconRect.setTop(opt.rect.top() + 5);
        iconRect.setWidth(32);
        iconRect.setHeight(32);
        
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            // Use appropriate icon mode based on state
            QIcon::Mode iconMode = (opt.state & QStyle::State_Selected) ? QIcon::Selected : QIcon::Normal;
            if (opt.state & QStyle::State_MouseOver && !(opt.state & QStyle::State_Selected)) {
                iconMode = QIcon::Active;
            }
            icon.paint(painter, iconRect, Qt::AlignCenter, iconMode);
        }
        
        // Draw text aligned to right of icon
        QRect textRect = opt.rect;
        textRect.setLeft(iconRect.right() + 10);
        textRect.setTop(opt.rect.top() + 5);
        textRect.setRight(opt.rect.right() - 5);
        textRect.setBottom(opt.rect.bottom() - 5);
        
        painter->setPen(opt.palette.color(opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, index.data(Qt::DisplayRole).toString());
    }
    
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(80); // Fixed height for multi-line text
        return size;
    }
};

class TextureListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit TextureListWidget(QWidget *parent = nullptr);
    
    void addTexture(const LibTXD::Texture* texture, const uint8_t* data, int index);
    void updateTexture(const LibTXD::Texture* texture, const uint8_t* data, int index);
    void clearTextures();

signals:
    void exportRequested(int index);
    void importRequested(int index);
    void replaceDiffuseRequested(int index);
    void replaceAlphaRequested(int index);
    void removeRequested(int index);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QString formatTextureInfo(const LibTXD::Texture* texture) const;
    QPixmap createThumbnail(const LibTXD::Texture* texture, const uint8_t* data) const;
};

#endif // TEXTURELISTWIDGET_H

