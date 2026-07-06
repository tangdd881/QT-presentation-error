#ifndef STORYGRAPHSCENE_H
#define STORYGRAPHSCENE_H

#include <QGraphicsScene>
#include <QMap>
#include <QPointer>
#include "StoryGraph.h"

class NodeGraphicsItem;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;

class StoryGraphScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit StoryGraphScene(StoryGraph *graph, QObject *parent = nullptr);

    QString selectedNodeId() const;
    NodeGraphicsItem* findNodeItem(const QString &nodeId) const;
    void addRootNode();
    QString getRootNodeId() const;
    void refreshFromGraph(bool useLayout);
    void refreshFromGraph();
    bool isOperationInProgress() const { return m_operationInProgress;}

signals:
    void nodeSelected(const QString &nodeId);
    void linkToExistingNodeRequested(const QString &branchNodeId);  // 新增

public slots:
    void onAddChildNode(NodeGraphicsItem *parentItem, bool isChoice);
    void onDeleteNode(NodeGraphicsItem *item);
    void onRenameNode(NodeGraphicsItem *item);
    void onNodeDragFinished(NodeGraphicsItem *item);   // 新增
    void onChangeNodeType(NodeGraphicsItem *item);

private:
    void buildScene();
    void layoutTree();
    void updateConnectionsAndJumpIndicators();
    NodeGraphicsItem* findNearestNode(NodeGraphicsItem *source, qreal maxDistance);
    bool isDescendant(const QString &ancestorId, const QString &descendantId);

    // 可选：刷新连线（不重建整个场景）
    void refreshConnectionsOnly();
    StoryGraph *m_graph;
    QMap<QString, QPointer<NodeGraphicsItem>> m_nodeItems;
    QList<QGraphicsLineItem*> m_connections;
    QList<QGraphicsItem*> m_jumpIndicators;
    QString m_currentSelectedNodeId;
    bool m_suppressLayout = false;
    bool m_operationInProgress = false;
};

#endif // STORYGRAPHSCENE_H