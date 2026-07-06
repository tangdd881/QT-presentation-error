#include "StoryGraphView.h"
#include "NodeGraphicsItem.h"
#include "StoryGraphScene.h"
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <qmath.h>
#include <QMouseEvent>

StoryGraphView::StoryGraphView(StoryGraphScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent), m_scene(scene), m_zoom(1.0)
{
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void StoryGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem *item = scene()->itemAt(scenePos, QTransform());
    NodeGraphicsItem *nodeItem = dynamic_cast<NodeGraphicsItem*>(item);

    QMenu menu;

    if (nodeItem) {
        QAction *deleteAct = menu.addAction(tr("删除节点"));
        QAction *renameAct = menu.addAction(tr("重命名"));
        QAction *changeTypeAct = menu.addAction(tr("修改节点类型"));   // 新增
        QAction *selected = menu.exec(event->globalPos());

        if (selected == deleteAct) {
            m_scene->onDeleteNode(nodeItem);
        } else if (selected == renameAct) {
            m_scene->onRenameNode(nodeItem);
        } else if (selected == changeTypeAct) {
            m_scene->onChangeNodeType(nodeItem);   // 新增调用
        }
    } else {
        // 空白处右键菜单不变
        if (m_scene->getRootNodeId().isEmpty()) {
            QAction *addRootAct = menu.addAction(tr("添加根节点"));
            if (menu.exec(event->globalPos()) == addRootAct) {
                m_scene->addRootNode();
            }
        } else {
            menu.addAction(tr("请选中节点后操作"))->setEnabled(false);
            menu.exec(event->globalPos());
        }
    }
}

void StoryGraphView::setZoomLevel(qreal zoom)
{
    zoom = qBound(0.1, zoom, 5.0);
    if (qFuzzyCompare(m_zoom, zoom))
        return;

    qreal factor = zoom / m_zoom;
    scale(factor, factor);
    m_zoom = zoom;
    emit zoomChanged(m_zoom);
}

void StoryGraphView::fitToView()
{
    if (!scene() || scene()->items().isEmpty()) {
        // 无内容时重置变换
        resetTransform();
        m_zoom = 1.0;
        emit zoomChanged(m_zoom);
        return;
    }

    QRectF sceneRect = scene()->itemsBoundingRect();
    // 添加一些边距
    sceneRect.adjust(-50, -50, 50, 50);
    fitInView(sceneRect, Qt::KeepAspectRatio);
    // 更新 m_zoom 值（fitInView 后获取 transform）
    qreal newZoom = transform().m11();
    m_zoom = newZoom;
    emit zoomChanged(m_zoom);
}

void StoryGraphView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        double delta = event->angleDelta().y();
        double factor = (delta > 0) ? 1.1 : 0.9;
        qreal newZoom = m_zoom * factor;
        newZoom = qBound(0.1, newZoom, 5.0);
        setZoomLevel(newZoom);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void StoryGraphView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    // 可选：窗口大小改变时自动适应？不强制，保持当前缩放
}

void StoryGraphView::updateZoom(qreal factor)
{
    scale(factor, factor);
    m_zoom *= factor;
    emit zoomChanged(m_zoom);
}

void StoryGraphView::adjustToCenter()
{
    // 可选：滚动到场景中心
    if (scene()) {
        centerOn(scene()->sceneRect().center());
    }
}
//鼠标调整视角位置实现

void StoryGraphView::mousePressEvent(QMouseEvent *event)
{
    // 按住 Alt 键 + 左键开始拖拽平移
    if ((event->modifiers() & Qt::AltModifier) && (event->button() == Qt::LeftButton)) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    // 其他情况交给基类处理（包括框选、点击节点等）
    QGraphicsView::mousePressEvent(event);
}

void StoryGraphView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        QPoint delta = event->pos() - m_lastPanPos;
        // 移动滚动条来实现视图平移
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPos = event->pos();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void StoryGraphView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_panning && event->button() == Qt::LeftButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}