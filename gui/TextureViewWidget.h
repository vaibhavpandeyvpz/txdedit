#ifndef TEXTUREVIEWWIDGET_H
#define TEXTUREVIEWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QSlider>
#include <QLabel>

class TextureViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit TextureViewWidget(QWidget *parent = nullptr);
    void setPixmap(const QPixmap& pixmap);
    void clear();
    
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoom100();
    void resetView();
    
    bool hasBeenShownOnce() const { return hasBeenShown; }
    void setHasBeenShownOnce(bool shown) { hasBeenShown = shown; }
    void resetHasBeenShown() { hasBeenShown = false; }

signals:
    void zoomChanged(double factor);

private:
    void setupUI();
    void updateZoomLabel();
    void setZoomFactor(double factor);
    void updateFloatingControlsPosition();
    
    QGraphicsView* graphicsView;
    QGraphicsScene* scene;
    QGraphicsPixmapItem* pixmapItem;
    
    QWidget* floatingControls;
    QToolButton* zoomInBtn;
    QToolButton* zoomOutBtn;
    QToolButton* zoomFitBtn;
    QToolButton* resetBtn;
    QLabel* zoomLabel;
    
    double currentZoom;
    bool isPanning;
    QPoint lastPanPoint;
    bool hasBeenShown;
    
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // TEXTUREVIEWWIDGET_H

