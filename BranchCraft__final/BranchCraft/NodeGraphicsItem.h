// NodeGraphicsItem.h
#pragma once
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

class NodeGraphicsItem :public QObject,public QGraphicsRectItem
{
    Q_OBJECT
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    explicit NodeGraphicsItem(const QString &nodeId, const QString &name, bool isChoice, QGraphicsItem *parent = nullptr);

    QString nodeId() const { return m_nodeId; }
    QString nodeName() const { return m_name; }
    void setNodeName(const QString &name);
    void setSelected(bool selected);

    // 加号按钮区域（用于命中测试）
    QRectF plusButtonRect() const;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void plusButtonClicked(NodeGraphicsItem *item);
    void selected(NodeGraphicsItem *item);
    void positionChanged(NodeGraphicsItem *item);   // 拖拽移动时发射
    void dragFinished(NodeGraphicsItem *item);      // 拖拽结束发射

private:
    QString m_nodeId;
    QString m_name;
    bool m_isChoice;      // 分支节点 vs 普通节点
    bool m_isSelected;
    QRectF m_rect;
    QRectF m_plusRect;    // 加号图标区域
    bool m_dragging = false;
    QPointF m_dragStartPos;
    QPointF m_itemStartPos;
};