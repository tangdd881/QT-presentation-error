// NodeGraphicsItem.cpp
#include "NodeGraphicsItem.h"
#include <QPen>
#include <QBrush>
#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>

NodeGraphicsItem::NodeGraphicsItem(const QString &nodeId, const QString &name, bool isChoice, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_nodeId(nodeId), m_name(name), m_isChoice(isChoice), m_isSelected(false)
{
    setFlags(ItemIsSelectable | ItemIsFocusable);
    setAcceptHoverEvents(true);

    // 计算节点尺寸（根据文本长度）
    QFont font;
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(name) + 40;
    int textHeight = fm.height() + 20;
    setRect(0, 0, qMax(120, textWidth), textHeight);
    setPen(QPen(Qt::black, 1.5));
    setBrush(QBrush(QColor(240, 240, 240)));

    // 加号按钮区域（右下角）
    m_plusRect = QRectF(rect().width() - 20, rect().height() - 20, 16, 16);
}

void NodeGraphicsItem::setNodeName(const QString &name)
{
    m_name = name;
    QFontMetrics fm((QFont()));
    int textWidth = fm.horizontalAdvance(name) + 40;
    int textHeight = fm.height() + 20;
    prepareGeometryChange();
    m_rect = QRectF(0, 0, qMax(120, textWidth), textHeight);
    m_plusRect = QRectF(m_rect.width() - 20, m_rect.height() - 20, 16, 16);
    update();
}
void NodeGraphicsItem::setSelected(bool selected)
{
    m_isSelected = selected;
    QGraphicsRectItem::setSelected(selected);
    if (selected) {
        setPen(QPen(Qt::blue, 2));
        setBrush(QBrush(QColor(200, 220, 255)));
    } else {
        setPen(QPen(Qt::black, 1.5));
        setBrush(QBrush(QColor(240, 240, 240)));
    }
    update();
}

void NodeGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    // 绘制节点名称
    painter->drawText(rect(), Qt::AlignCenter, m_name);

    // 绘制类型图标（普通节点或分支节点小标记）
    if (m_isChoice) {
        painter->setBrush(Qt::darkYellow);
        painter->drawEllipse(rect().topLeft() + QPointF(5, 5), 6, 6);
    } else {
        painter->setBrush(Qt::darkGreen);
        painter->drawEllipse(rect().topLeft() + QPointF(5, 5), 4, 4);
    }

    // 绘制加号小圆圈
    painter->setBrush(Qt::white);
    painter->setPen(Qt::black);
    painter->drawEllipse(m_plusRect);
    painter->drawLine(m_plusRect.center() + QPointF(-4, 0), m_plusRect.center() + QPointF(4, 0));
    painter->drawLine(m_plusRect.center() + QPointF(0, -4), m_plusRect.center() + QPointF(0, 4));
}

QRectF NodeGraphicsItem::plusButtonRect() const
{
    return mapToScene(m_plusRect).boundingRect();
}

void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 加号按钮优先
    if (m_plusRect.contains(event->pos())) {
        emit plusButtonClicked(this);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::AltModifier)) {
        // 发射选中信号（由场景处理选中样式和属性面板更新）
        emit selected(this);
        // 开始拖拽（拖拽不影响选中，只移动位置）
        m_dragging = true;
        m_dragStartPos = event->scenePos();
        m_itemStartPos = pos();
        event->accept();
        return;
    }

    QGraphicsRectItem::mousePressEvent(event);
}

void NodeGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragging) {
        QPointF newPos = m_itemStartPos + (event->scenePos() - m_dragStartPos);
        setPos(newPos);
        emit positionChanged(this);
        event->accept();
        return;
    }
    QGraphicsRectItem::mouseMoveEvent(event);
}

void NodeGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        emit dragFinished(this);
        event->accept();
        return;
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}