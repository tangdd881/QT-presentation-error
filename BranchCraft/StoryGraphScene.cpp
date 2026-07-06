#include "StoryGraphScene.h"
#include "NodeGraphicsItem.h"
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QUuid>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>

StoryGraphScene::StoryGraphScene(StoryGraph *graph, QObject *parent)
    : QGraphicsScene(parent), m_graph(graph)
{
}
void StoryGraphScene::refreshFromGraph(bool useLayout)
{
    QString savedSelectedId = m_currentSelectedNodeId;
    QMap<QString, QPointF> savedPositions;
    if (!useLayout) {
        for (auto it = m_nodeItems.begin(); it != m_nodeItems.end(); ++it) {
            if (it.value()) savedPositions[it.key()] = it.value()->pos();
        }
    }

    clear();
    m_nodeItems.clear();
    m_connections.clear();
    m_jumpIndicators.clear();
    m_currentSelectedNodeId.clear();

    if (!m_graph) return;

    buildScene();

    if (useLayout) {
        layoutTree();
    } else {
        for (auto it = m_nodeItems.begin(); it != m_nodeItems.end(); ++it) {
            if (savedPositions.contains(it.key())) {
                it.value()->setPos(savedPositions[it.key()]);
            }
        }
    }
    updateConnectionsAndJumpIndicators();

    // 恢复选中状态
    if (!savedSelectedId.isEmpty() && m_nodeItems.contains(savedSelectedId)) {
        for (auto &item : m_nodeItems) if (item) item->setSelected(false);
        auto item = m_nodeItems[savedSelectedId];
        item->setSelected(true);
        m_currentSelectedNodeId = savedSelectedId;
        emit nodeSelected(savedSelectedId);
    } else {
        QString rootId = m_graph->getRootId();
        if (!rootId.isEmpty() && m_nodeItems.contains(rootId)) {
            for (auto &item : m_nodeItems) item->setSelected(false);
            m_nodeItems[rootId]->setSelected(true);
            m_currentSelectedNodeId = rootId;
            emit nodeSelected(rootId);
        } else {
            m_currentSelectedNodeId.clear();
        }
    }
}

void StoryGraphScene::refreshFromGraph()
{
    refreshFromGraph(true);   // 默认进行自动布局
}



void StoryGraphScene::buildScene()
{

    QList<NodeData> nodes = m_graph->getAllNodes();
    for (const NodeData &node : nodes) {
        auto item = new NodeGraphicsItem(node.id, node.name, node.type == NodeType::Choice);
        connect(item, &NodeGraphicsItem::selected, this, [this](NodeGraphicsItem *it){
            // 清除所有节点的选中样式
            for (auto &nodeItem : m_nodeItems) {
                if (nodeItem) nodeItem->setSelected(false);
            }
            // 设置当前节点选中
            it->setSelected(true);
            m_currentSelectedNodeId = it->nodeId();
            emit nodeSelected(it->nodeId());
        });
        connect(item, &NodeGraphicsItem::positionChanged, this, &StoryGraphScene::refreshConnectionsOnly);
        connect(item, &NodeGraphicsItem::dragFinished, this, &StoryGraphScene::onNodeDragFinished);
        connect(item, &NodeGraphicsItem::plusButtonClicked, [this, item](){
            if (m_operationInProgress) return;
            m_operationInProgress = true;

            // 先弹出菜单（不预先选中节点）
            NodeData parentNode = m_graph->getNode(item->nodeId());
            QMenu menu;
            QAction *actNormal = nullptr;
            QAction *actChoice = nullptr;
            QAction *actLinkExisting = nullptr;

            if (parentNode.type == NodeType::Normal) {
                actNormal = menu.addAction(tr("添加普通子节点"));
                actChoice = menu.addAction(tr("添加分支子节点"));
            } else { // 分支节点
                actNormal = menu.addAction(tr("新建普通节点并链接"));
                actChoice = menu.addAction(tr("新建分支节点并链接"));
                menu.addSeparator();
                actLinkExisting = menu.addAction(tr("链接到已有节点（不创建新节点）"));
            }

            QAction *selected = menu.exec(QCursor::pos());
            if (!selected) {
                m_operationInProgress = false;
                return;
            }

            // 用户做出选择后，再选中当前节点（如果尚未选中）
            if (m_currentSelectedNodeId != item->nodeId()) {
                for (auto &nodeItem : m_nodeItems) {
                    if (nodeItem) nodeItem->setSelected(false);
                }
                item->setSelected(true);
                m_currentSelectedNodeId = item->nodeId();
                emit nodeSelected(item->nodeId());   // 通知属性面板
            }

            // 执行对应操作
            if (selected == actNormal || selected == actChoice) {
                bool isChoiceChild = (selected == actChoice);
                onAddChildNode(item, isChoiceChild);
            } else if (selected == actLinkExisting) {
                emit linkToExistingNodeRequested(item->nodeId());
            }

            m_operationInProgress = false;
        });
        addItem(item);
        m_nodeItems[node.id] = item;
    }
}

void StoryGraphScene::layoutTree()
{
    QString rootId = m_graph->getRootId();
    if (rootId.isEmpty()) return;

    // 节点尺寸和间距
    const int NODE_WIDTH = 150;
    const int H_SPACING = 80;
    const int V_SPACING = 100;

    // 递归计算子树宽度（所有子孙节点的累计宽度）
    std::function<double(const QString&)> calcSubtreeWidth = [&](const QString &id) -> double {
        NodeData node = m_graph->getNode(id);
        double total = 0;
        for (const QString &childId : node.childIds) {
            total += calcSubtreeWidth(childId);
        }
        if (node.childIds.isEmpty()) {
            total = 1;  // 叶子节点权重为1
        }
        return total;
    };

    // 递归设置节点位置，返回当前子树占用的总宽度（用于兄弟节点偏移）
    std::function<double(const QString&, int, double&)> setPos = [&](const QString &id, int depth, double &xOffset) -> double {
        NodeGraphicsItem *item = m_nodeItems.value(id);
        if (!item) return 0;

        double subtreeWidth = calcSubtreeWidth(id);
        double nodeWidth = NODE_WIDTH;
        double totalWidth = subtreeWidth * (nodeWidth + H_SPACING) - H_SPACING;

        // 当前节点的 x 坐标 = xOffset + 子树宽度的一半 - 自身宽度的一半
        double x = xOffset + totalWidth / 2 - nodeWidth / 2;
        double y = depth * V_SPACING;
        item->setPos(x, y);

        // 递归放置子节点，并累计偏移
        double childXOffset = xOffset;
        NodeData node = m_graph->getNode(id);
        for (const QString &childId : node.childIds) {
            double childTotalWidth = setPos(childId, depth + 1, childXOffset);
            // 更新偏移量
            childXOffset += childTotalWidth + H_SPACING;
        }

        return totalWidth;
    };

    double startX = 0;
    setPos(rootId, 0, startX);
}

void StoryGraphScene::updateConnectionsAndJumpIndicators()
{
    // 清除旧元素
    for (auto line : m_connections) {
        removeItem(line);
        delete line;
    }
    m_connections.clear();
    for (auto indicator : m_jumpIndicators) {
        removeItem(indicator);
        delete indicator;
    }
    m_jumpIndicators.clear();

    QList<NodeData> nodes = m_graph->getAllNodes();

    // 1. 绘制所有父子连线（包括普通节点和分支节点）
    for (const NodeData &node : nodes) {
        NodeGraphicsItem *parentItem = m_nodeItems.value(node.id);
        if (!parentItem) continue;
        QPointF parentCenter = parentItem->scenePos() + parentItem->boundingRect().center();
        for (const QString &childId : node.childIds) {
            NodeGraphicsItem *childItem = m_nodeItems.value(childId);
            if (childItem) {
                QPointF childCenter = childItem->scenePos() + childItem->boundingRect().center();
                QGraphicsLineItem *line = new QGraphicsLineItem(parentCenter.x(), parentCenter.y(),
                                                                childCenter.x(), childCenter.y());
                line->setPen(QPen(Qt::gray, 2));
                addItem(line);
                m_connections.append(line);
            }
        }
    }

    // 2. 为分支节点绘制跳转标注（仅当目标节点不是其子节点时）
    for (const NodeData &node : nodes) {
        if (node.type != NodeType::Choice) continue;
        NodeGraphicsItem *parentItem = m_nodeItems.value(node.id);
        if (!parentItem) continue;

        QPointF basePos = parentItem->scenePos() + QPointF(parentItem->boundingRect().width(), 0);
        int yOffset = 0;
        const int stepY = 25;

        for (const auto &choice : node.choices) {
            if (choice.targetNodeId.isEmpty()) continue;
            // 如果目标已经是子节点，则已经通过连线表示，不再重复绘制标注
            if (node.childIds.contains(choice.targetNodeId)) continue;

            NodeGraphicsItem *targetItem = m_nodeItems.value(choice.targetNodeId);
            if (!targetItem) continue; // 防止崩溃

            QString displayText = QString("➜ 跳转到：%1").arg(targetItem->nodeName());
            QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(displayText);
            textItem->setPos(basePos.x() + 10, basePos.y() + yOffset);
            textItem->setBrush(Qt::gray);
            textItem->setToolTip(choice.text);
            addItem(textItem);
            m_jumpIndicators.append(textItem);
            yOffset += stepY;
        }
    }
}

QString StoryGraphScene::selectedNodeId() const
{
    return m_currentSelectedNodeId;
}

NodeGraphicsItem* StoryGraphScene::findNodeItem(const QString &nodeId) const
{
    return m_nodeItems.value(nodeId, nullptr);
}

void StoryGraphScene::onAddChildNode(NodeGraphicsItem *parentItem, bool isChoiceChild)
{
    if (!parentItem) return;
    QString parentId = parentItem->nodeId();
    NodeData parentNode = m_graph->getNode(parentId);

    if (parentNode.type == NodeType::Normal && !parentNode.childIds.isEmpty()) {
        QMessageBox::warning(nullptr, tr("错误"), tr("普通节点只能有一个子节点"));
        return;
    }

    NodeData newNode;
    newNode.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newNode.type = isChoiceChild ? NodeType::Choice : NodeType::Normal;
    newNode.name = tr("新节点");
    m_graph->addNode(newNode, parentId);   // 建立父子关系

    // 如果父节点是分支节点，自动添加一个指向新子节点的选项（这样在运行时也能跳转）
    if (parentNode.type == NodeType::Choice) {
        NodeData updatedParent = m_graph->getNode(parentId);
        NodeData::Choice newChoice;
        newChoice.text = tr("选项 %1").arg(updatedParent.choices.size() + 1);
        newChoice.targetNodeId = newNode.id;
        updatedParent.choices.append(newChoice);
        m_graph->updateNode(updatedParent);
    }

    refreshFromGraph();
    emit nodeSelected(newNode.id);
    if (m_currentSelectedNodeId == parentId) {
        // 重新发射节点选中信号，以便属性面板重新加载节点数据（包含新选项）
        emit nodeSelected(parentId);
    }
}


void StoryGraphScene::onDeleteNode(NodeGraphicsItem *item)
{

    if (!item) return;
    m_graph->removeNode(item->nodeId());
    refreshFromGraph();
}

void StoryGraphScene::onRenameNode(NodeGraphicsItem *item)
{
    \
    if (!item) return;
    bool ok;
    QString newName = QInputDialog::getText(QApplication::activeWindow(),
                                            "重命名节点",
                                            "新名称:",
                                            QLineEdit::Normal,
                                            item->nodeName(),
                                            &ok);
    if (ok && !newName.isEmpty()) {
        NodeData node = m_graph->getNode(item->nodeId());
        node.name = newName;
        m_graph->updateNode(node);
        item->setNodeName(newName);
        updateConnectionsAndJumpIndicators();
    }

}

QString StoryGraphScene::getRootNodeId() const
{
    return m_graph ? m_graph->getRootId() : QString();
}

void StoryGraphScene::addRootNode()
{
    NodeData newNode;
    newNode.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newNode.type = NodeType::Normal;
    newNode.name = "根节点";
    m_graph->addNode(newNode, QString());
    refreshFromGraph();
}

//节点拖拽功能

NodeGraphicsItem* StoryGraphScene::findNearestNode(NodeGraphicsItem *source, qreal maxDistance)
{
    NodeGraphicsItem *nearest = nullptr;
    qreal minDist = maxDistance;
    for (auto it = m_nodeItems.begin(); it != m_nodeItems.end(); ++it) {
        NodeGraphicsItem *item = it.value();
        if (item == source) continue;
        QPointF srcCenter = source->scenePos() + source->boundingRect().center();
        QPointF dstCenter = item->scenePos() + item->boundingRect().center();
        qreal dist = QLineF(srcCenter, dstCenter).length();
        if (dist < minDist) {
            minDist = dist;
            nearest = item;
        }
    }
    return nearest;
}

bool StoryGraphScene::isDescendant(const QString &ancestorId, const QString &descendantId)
{
    QList<QString> toCheck;
    toCheck.append(descendantId);
    while (!toCheck.isEmpty()) {
        QString cur = toCheck.takeFirst();
        NodeData node = m_graph->getNode(cur);
        if (node.parentId == ancestorId) return true;
        if (!node.parentId.isEmpty()) toCheck.append(node.parentId);
    }
    return false;
}

void StoryGraphScene::refreshConnectionsOnly()
{
    // 清除旧连线
    for (auto line : m_connections) {
        removeItem(line);
        delete line;
    }
    m_connections.clear();
    for (auto indicator : m_jumpIndicators) {
        removeItem(indicator);
        delete indicator;
    }
    m_jumpIndicators.clear();

    // 重新绘制连线（与 updateConnectionsAndJumpIndicators 内容相同）
    QList<NodeData> nodes = m_graph->getAllNodes();
    for (const NodeData &node : nodes) {
        NodeGraphicsItem *parentItem = m_nodeItems.value(node.id);
        if (!parentItem) continue;
        QPointF parentCenter = parentItem->scenePos() + parentItem->boundingRect().center();
        for (const QString &childId : node.childIds) {
            NodeGraphicsItem *childItem = m_nodeItems.value(childId);
            if (childItem) {
                QPointF childCenter = childItem->scenePos() + childItem->boundingRect().center();
                QGraphicsLineItem *line = new QGraphicsLineItem(parentCenter.x(), parentCenter.y(),
                                                                childCenter.x(), childCenter.y());
                line->setPen(QPen(Qt::gray, 2));
                addItem(line);
                m_connections.append(line);
            }
        }
    }
    // 绘制跳转标注（复用原有逻辑，可提取公共函数）
    for (const NodeData &node : nodes) {
        if (node.type != NodeType::Choice) continue;
        NodeGraphicsItem *parentItem = m_nodeItems.value(node.id);
        if (!parentItem) continue;
        QPointF basePos = parentItem->scenePos() + QPointF(parentItem->boundingRect().width(), 0);
        int yOffset = 0;
        const int stepY = 25;
        for (const auto &choice : node.choices) {
            if (choice.targetNodeId.isEmpty()) continue;
            if (node.childIds.contains(choice.targetNodeId)) continue;
            NodeGraphicsItem *targetItem = m_nodeItems.value(choice.targetNodeId);
            if (!targetItem) continue;
            QString displayText = QString("➜ 跳转到：%1").arg(targetItem->nodeName());
            QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(displayText);
            textItem->setPos(basePos.x() + 10, basePos.y() + yOffset);
            textItem->setBrush(Qt::gray);
            textItem->setToolTip(choice.text);
            addItem(textItem);
            m_jumpIndicators.append(textItem);
            yOffset += stepY;
        }
    }
}

void StoryGraphScene::onNodeDragFinished(NodeGraphicsItem *draggedItem)
{
    if (!draggedItem) return;
    QString draggedId = draggedItem->nodeId();
    NodeData draggedNode = m_graph->getNode(draggedId);
    if (draggedNode.id.isEmpty()) return;

    // 动态阈值：节点宽度的1.2倍或最小100
    qreal threshold = qMax(100.0, draggedItem->boundingRect().width() * 1.8);
    NodeGraphicsItem *targetItem = findNearestNode(draggedItem, threshold);
    bool treeChanged = false;

    if (targetItem) {
        QPointF srcCenter = draggedItem->scenePos() + draggedItem->boundingRect().center();
        QPointF dstCenter = targetItem->scenePos() + targetItem->boundingRect().center();
        qreal dist = QLineF(srcCenter, dstCenter).length();
        qDebug() << "Nearest node:" << targetItem->nodeId() << "distance:" << dist << "threshold:" << threshold;
    } else {
        qDebug() << "No node within threshold, detach from parent";
    }

    if (!targetItem) {
        // 太远：断开父连接
        if (!draggedNode.parentId.isEmpty()) {
            NodeData oldParent = m_graph->getNode(draggedNode.parentId);
            if (!oldParent.id.isEmpty()) {
                oldParent.childIds.removeAll(draggedId);
                m_graph->updateNode(oldParent);
            }
            draggedNode.parentId = QString();
            m_graph->updateNode(draggedNode);
            treeChanged = true;
        }
    } else {
        QString targetId = targetItem->nodeId();
        NodeData targetNode = m_graph->getNode(targetId);
        if (targetId != draggedId && !isDescendant(targetId, draggedId)) {
            if (targetNode.type == NodeType::Normal && !targetNode.childIds.isEmpty()) {
                // 普通节点已有子节点，无法添加
                // 这里不改变树结构
            } else {
                // 断开原父节点
                if (!draggedNode.parentId.isEmpty()) {
                    NodeData oldParent = m_graph->getNode(draggedNode.parentId);
                    if (!oldParent.id.isEmpty()) {
                        oldParent.childIds.removeAll(draggedId);
                        m_graph->updateNode(oldParent);
                    }
                }
                // 添加到新父节点
                targetNode.childIds.append(draggedId);
                draggedNode.parentId = targetId;
                m_graph->updateNode(targetNode);
                m_graph->updateNode(draggedNode);
                treeChanged = true;

                // 如果目标节点是分支节点，自动添加一个选项
                if (targetNode.type == NodeType::Choice) {
                    NodeData updatedTarget = m_graph->getNode(targetId);
                    bool alreadyHas = false;
                    for (const auto &ch : updatedTarget.choices) {
                        if (ch.targetNodeId == draggedId) {
                            alreadyHas = true;
                            break;
                        }
                    }
                    if (!alreadyHas) {
                        NodeData::Choice newChoice;
                        newChoice.text = tr("跳转到 %1").arg(draggedItem->nodeName());
                        newChoice.targetNodeId = draggedId;
                        newChoice.buttonPos = QPointF(100, 100 + updatedTarget.choices.size() * 120);
                        newChoice.buttonSize = QSizeF(400, 100);
                        newChoice.textFontSize = 20;
                        newChoice.textColor = Qt::white;
                        updatedTarget.choices.append(newChoice);
                        m_graph->updateNode(updatedTarget);
                    }
                }
            }
        }
    }

    if (treeChanged) {
        refreshFromGraph(false);   // 保留位置刷新
        // 刷新属性面板
        if (m_currentSelectedNodeId == draggedItem->nodeId()) {
            emit nodeSelected(draggedItem->nodeId());
        }
    }

    // 确保拖拽的节点是选中的（如果树没有改变，但可能选中状态丢失）
    if (m_currentSelectedNodeId != draggedId) {
        // 清除其他节点选中样式
        for (auto &item : m_nodeItems) {
            if (item && item != draggedItem) item->setSelected(false);
        }
        draggedItem->setSelected(true);
        m_currentSelectedNodeId = draggedId;
        // 如果树没改变，但选中节点变化，仍需刷新属性面板
        if (!treeChanged) {
            emit nodeSelected(draggedId);
        }
    } else {
        // 已经是当前选中节点，但树结构改变时需要刷新属性面板
        if (treeChanged) {
            emit nodeSelected(draggedId);
        }
    }
}

void StoryGraphScene::onChangeNodeType(NodeGraphicsItem *item)
{
    if (!item) return;
    QString nodeId = item->nodeId();
    NodeData node = m_graph->getNode(nodeId);
    if (node.id.isEmpty()) return;

    NodeType oldType = node.type;
    NodeType newType = (oldType == NodeType::Normal) ? NodeType::Choice : NodeType::Normal;

    // 约束检查
    if (oldType == NodeType::Normal && newType == NodeType::Choice) {
        // 普通节点 -> 分支节点：要求只有一张幻灯片
        if (node.slides.size() != 1) {
            QMessageBox::warning(nullptr, tr("无法转换"),
                                 tr("普通节点包含 %1 张幻灯片，分支节点只能有一张幻灯片。\n请删除多余幻灯片后再试。").arg(node.slides.size()));
            return;
        }
    } else if (oldType == NodeType::Choice && newType == NodeType::Normal) {
        // 分支节点 -> 普通节点：要求最多只有一个子节点
        if (node.childIds.size() > 1) {
            QMessageBox::warning(nullptr, tr("无法转换"),
                                 tr("分支节点有 %1 个子节点，普通节点只能有一个子节点。\n请删除多余子节点后再试。").arg(node.childIds.size()));
            return;
        }
    }

    // 执行类型转换
    node.type = newType;
    // 分支节点转普通节点时，如果没有子节点但存在 choices，可以选择清空 choices 或保留？保留没有意义，清空。
    if (newType == NodeType::Normal) {
        node.choices.clear();  // 分支选项不再需要
    }
    // 普通节点转分支节点时，已有的子节点会自动成为分支节点的子节点（childIds 保留），但不自动生成选项，用户可手动添加。
    // 如果想自动生成一个选项指向第一个子节点，可以添加以下代码：
    if (newType == NodeType::Choice && !node.childIds.isEmpty()) {
        NodeData::Choice autoChoice;
        autoChoice.text = tr("选项1");
        autoChoice.targetNodeId = node.childIds.first();
        autoChoice.buttonSize = QSizeF(400, 100);
        autoChoice.textFontSize = 20;
        autoChoice.textColor = Qt::white;
        node.choices.append(autoChoice);
    }

    m_graph->updateNode(node);
    // 刷新场景，保留节点位置（不重新布局）
    refreshFromGraph(false);
}
