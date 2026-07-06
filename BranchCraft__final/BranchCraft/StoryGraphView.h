// StoryGraphView.h (修改部分)
#ifndef STORYGRAPHVIEW_H
#define STORYGRAPHVIEW_H

#include <QGraphicsView>
#include "NodeGraphicsItem.h"

class StoryGraphScene;

class StoryGraphView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit StoryGraphView(StoryGraphScene *scene, QWidget *parent = nullptr);

    void setZoomLevel(qreal zoom);
    qreal zoomLevel() const { return m_zoom; }
    void fitToView();

signals:
    void zoomChanged(qreal zoom);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // 新增：鼠标事件用于拖拽平移
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateZoom(qreal factor);
    void adjustToCenter();

    StoryGraphScene *m_scene;
    qreal m_zoom;

    // 新增成员：拖拽平移状态
    bool m_panning = false;
    QPoint m_lastPanPos;
};

#endif // STORYGRAPHVIEW_H