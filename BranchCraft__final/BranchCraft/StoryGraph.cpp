#include "StoryGraph.h"
#include <QDebug>

StoryGraph::StoryGraph(QObject *parent) : QObject(parent) {}

void StoryGraph::addNode(const NodeData &node, const QString &parentId)

{
    QMutexLocker locker(&m_mutex);
    if (m_nodes.contains(node.id)) return;

    NodeData newNode = node;
    newNode.parentId = parentId;
    newNode.childIds.clear();

    // 如果是普通节点且幻灯片列表为空，则自动添加一个默认幻灯片
    // 在 addNode 函数中，将原来的普通节点判断改为：
    if (newNode.slides.isEmpty()) {
        Slide defaultSlide;
        defaultSlide.textRect = QRectF(1920/2 - 750, 1080 - 450, 1500, 400);
        defaultSlide.textFontSize = 20;
        defaultSlide.textColor = Qt::white;
        newNode.slides.append(defaultSlide);
    }

    m_nodes[newNode.id] = newNode;

    if (!parentId.isEmpty() && m_nodes.contains(parentId)) {
        m_nodes[parentId].childIds.append(newNode.id);
    } else if (parentId.isEmpty() && m_rootId.isEmpty()) {
        m_rootId = newNode.id;
    }
    emit dataChanged();
}

void StoryGraph::removeNode(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    if (!m_nodes.contains(id)) return;

    // 1. 递归删除所有子节点（包括子孙）
    QList<QString> children = m_nodes[id].childIds;
    for (const QString &childId : children) {
        removeSubtree(childId);
    }

    // 2. 从父节点中移除当前节点
    QString parentId = m_nodes[id].parentId;
    if (!parentId.isEmpty() && m_nodes.contains(parentId)) {
        m_nodes[parentId].childIds.removeAll(id);
    }

    // 3. 清理所有分支节点中指向当前节点的选项（避免残留）
    for (auto &node : m_nodes) {
        if (node.type == NodeType::Choice) {
            for (int i = node.choices.size() - 1; i >= 0; --i) {
                if (node.choices[i].targetNodeId == id) {
                    node.choices.removeAt(i);
                }
            }
        }
    }

    // 4. 删除自身
    m_nodes.remove(id);

    // 5. 如果删除的是根节点，重新设置根节点
    if (m_rootId == id) {
        m_rootId.clear();
        for (const NodeData &node : m_nodes) {
            if (node.parentId.isEmpty()) {
                m_rootId = node.id;
                break;
            }
        }
    }

    emit dataChanged();
}

void StoryGraph::removeSubtree(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    if (!m_nodes.contains(id)) return;
    for (const QString &childId : m_nodes[id].childIds) {
        removeSubtree(childId);
    }
    m_nodes.remove(id);
}

void StoryGraph::updateNode(const NodeData &node)
{
    QMutexLocker locker(&m_mutex);
    if (!m_nodes.contains(node.id)) return;
    // 保留原有父子关系，只更新其他字段
    NodeData updated = node;
    updated.parentId = m_nodes[node.id].parentId;
    updated.childIds = m_nodes[node.id].childIds;
    m_nodes[node.id] = updated;
    emit dataChanged();
}

void StoryGraph::moveNode(const QString &id, const QString &newParentId, int newIndex)
{
    QMutexLocker locker(&m_mutex);
    if (!m_nodes.contains(id)) return;
    NodeData node = m_nodes[id];
    QString oldParentId = node.parentId;

    if (oldParentId == newParentId) {
        // 同父节点内调整顺序
        if (newIndex >= 0 && newIndex != node.childIds.indexOf(id)) {
            QList<QString> &siblings = m_nodes[oldParentId].childIds;
            siblings.removeAll(id);
            if (newIndex >= siblings.size()) siblings.append(id);
            else siblings.insert(newIndex, id);
            m_nodes[oldParentId].childIds = siblings;
            emit dataChanged();
        }
        return;
    }

    // 移出旧父节点
    if (!oldParentId.isEmpty()) {
        m_nodes[oldParentId].childIds.removeAll(id);
    }
    // 移入新父节点
    node.parentId = newParentId;
    m_nodes[id] = node;
    if (!newParentId.isEmpty() && m_nodes.contains(newParentId)) {
        QList<QString> &siblings = m_nodes[newParentId].childIds;
        if (newIndex < 0 || newIndex >= siblings.size())
            siblings.append(id);
        else
            siblings.insert(newIndex, id);
    } else if (newParentId.isEmpty()) {
        // 成为根节点
        // 调整根节点顺序（如果有多个根，需要维护）
        // 简化：直接设为唯一根，如果已有根，则交换
        if (!m_rootId.isEmpty() && m_rootId != id) {
            // 将原根节点变为子节点？根据需求，简单处理：清空原根节点的根标志
            NodeData oldRoot = m_nodes[m_rootId];
            oldRoot.parentId = id;
            m_nodes[m_rootId] = oldRoot;
            m_rootId = id;
        } else {
            m_rootId = id;
        }
    }
    emit dataChanged();
}

NodeData StoryGraph::getNode(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    return m_nodes.value(id);
}

QList<NodeData> StoryGraph::getChildren(const QString &parentId) const
{
    QMutexLocker locker(&m_mutex);
    QList<NodeData> children;
    NodeData parent = m_nodes.value(parentId);
    for (const QString &childId : parent.childIds) {
        children.append(m_nodes.value(childId));
    }
    return children;
}

void StoryGraph::setRootId(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    if (m_nodes.contains(id) || id.isEmpty()) {
        m_rootId = id;
        emit dataChanged();
    }
}

void StoryGraph::clear()
{
    m_nodes.clear();
    m_rootId.clear();
    emit dataChanged();
}

void StoryGraph::rebuildChildIds()
{
    QMutexLocker locker(&m_mutex);
    // 清空所有childIds
    for (NodeData &node : m_nodes) {
        node.childIds.clear();
    }
    // 根据parentId重建
    for (NodeData &node : m_nodes) {
        if (!node.parentId.isEmpty() && m_nodes.contains(node.parentId)) {
            m_nodes[node.parentId].childIds.append(node.id);
        }
    }
    // 确保根节点有效
    if (!m_rootId.isEmpty() && !m_nodes.contains(m_rootId)) {
        m_rootId.clear();
        for (const NodeData &node : m_nodes) {
            if (node.parentId.isEmpty()) {
                m_rootId = node.id;
                break;
            }
        }
    }
}

QJsonObject StoryGraph::toJson() const
{
    QJsonObject obj;
    QJsonArray nodesArray;
    for (const NodeData &node : m_nodes) {
        QJsonObject nodeObj;
        nodeObj["id"] = node.id;
        nodeObj["type"] = (node.type == NodeType::Normal) ? "normal" : "choice";
        nodeObj["name"] = node.name;
        nodeObj["audio"] = node.audioPath;
        nodeObj["parentId"] = node.parentId;
        nodeObj["scenePosX"] = 0; // 不再使用
        nodeObj["scenePosY"] = 0;

        QJsonArray slidesArray;
        for (const Slide &slide : node.slides) {
            QJsonObject slideObj;
            slideObj["background"] = slide.backgroundImage;
            QJsonArray charsArray;
            for (const auto &ch : slide.characters) {
                QJsonObject chObj;
                chObj["name"] = ch.name;
                chObj["image"] = ch.imagePath;
                QJsonObject posObj;
                posObj["x"] = ch.position.x();
                posObj["y"] = ch.position.y();
                chObj["position"] = posObj;
                chObj["scale"] = ch.scale;

                // 名字文本属性
                QJsonObject nameRectObj;
                nameRectObj["x"] = ch.nameRect.x();
                nameRectObj["y"] = ch.nameRect.y();
                nameRectObj["w"] = ch.nameRect.width();
                nameRectObj["h"] = ch.nameRect.height();
                chObj["nameRect"] = nameRectObj;
                chObj["nameFontSize"] = ch.nameFontSize;
                chObj["nameColor"] = ch.nameColor.name();
                chObj["nameBgImage"] = ch.nameBgImage;
                charsArray.append(chObj);
            }
            slideObj["characters"] = charsArray;
            slideObj["text"] = slide.text;
            slideObj["duration"] = slide.duration;
            slideObj["textRectX"] = slide.textRect.x();
            slideObj["textRectY"] = slide.textRect.y();
            slideObj["textRectW"] = slide.textRect.width();
            slideObj["textRectH"] = slide.textRect.height();
            slideObj["textFontSize"] = slide.textFontSize;
            slideObj["textBgImage"] = slide.textBgImage;
            slideObj["textColor"] = slide.textColor.name();
            slideObj["bgmAction"] = static_cast<int>(slide.bgmAction);
            if (!slide.bgmPath.isEmpty())
                slideObj["bgmPath"] = slide.bgmPath;
            if (!slide.slideAudioPath.isEmpty())
                slideObj["slideAudioPath"] = slide.slideAudioPath;
            QJsonArray layerOrderArray;
            for (const QString &id : slide.layerOrder) {
                layerOrderArray.append(id);
            }
            slideObj["layerOrder"] = layerOrderArray;
            slidesArray.append(slideObj);
        }
        nodeObj["slides"] = slidesArray;

        if (node.type == NodeType::Choice) {
            QJsonArray choicesArray;
            for (const auto &ch : node.choices) {
                QJsonObject chObj;
                chObj["text"] = ch.text;
                chObj["image"] = ch.imagePath;
                chObj["targetId"] = ch.targetNodeId;
                QJsonObject posObj;
                posObj["x"] = ch.buttonPos.x();
                posObj["y"] = ch.buttonPos.y();
                chObj["position"] = posObj;
                chObj["textFontSize"] = ch.textFontSize;
                chObj["textColor"] = ch.textColor.name();
                chObj["buttonWidth"] = ch.buttonSize.width();
                chObj["buttonHeight"] = ch.buttonSize.height();
                choicesArray.append(chObj);
            }
            nodeObj["choices"] = choicesArray;
        }
        nodesArray.append(nodeObj);
    }
    obj["nodes"] = nodesArray;
    obj["rootId"] = m_rootId;
    return obj;
}

bool StoryGraph::fromJson(const QJsonObject &obj)
{
    clear();
    m_rootId = obj["rootId"].toString();
    QJsonArray nodesArray = obj["nodes"].toArray();
    QMap<QString, NodeData> tempNodes;
    for (const auto &val : nodesArray) {
        QJsonObject nodeObj = val.toObject();
        NodeData node;
        node.id = nodeObj["id"].toString();
        node.type = (nodeObj["type"].toString() == "choice") ? NodeType::Choice : NodeType::Normal;
        node.name = nodeObj["name"].toString();
        node.audioPath = nodeObj["audio"].toString();
        node.parentId = nodeObj["parentId"].toString();

        // slides
        QJsonArray slidesArray = nodeObj["slides"].toArray();
        for (const auto &slideVal : slidesArray) {
            QJsonObject slideObj = slideVal.toObject();
            Slide slide;
            slide.backgroundImage = slideObj["background"].toString();
            slide.text = slideObj["text"].toString();
            slide.textRect = QRectF(slideObj["textRectX"].toDouble(),
                                    slideObj["textRectY"].toDouble(),
                                    slideObj["textRectW"].toDouble(),
                                    slideObj["textRectH"].toDouble());
            slide.textFontSize = slideObj["textFontSize"].toInt(20);
            slide.textBgImage = slideObj["textBgImage"].toString();
            slide.textColor = QColor(slideObj["textColor"].toString());
            if (!slide.textColor.isValid()) slide.textColor = Qt::white;

            slide.duration = slideObj["duration"].toDouble();
            QJsonArray charsArray = slideObj["characters"].toArray();
            for (const auto &chVal : charsArray) {
                QJsonObject chObj = chVal.toObject();
                Slide::Character ch;
                ch.name = chObj["name"].toString("");
                ch.imagePath = chObj["image"].toString();
                QJsonObject posObj = chObj["position"].toObject();
                ch.position = QPointF(posObj["x"].toDouble(), posObj["y"].toDouble());
                ch.scale = chObj["scale"].toDouble();

                QJsonObject nameRectObj = chObj["nameRect"].toObject();
                ch.nameRect = QRectF(nameRectObj["x"].toDouble(),
                                     nameRectObj["y"].toDouble(),
                                     nameRectObj["w"].toDouble(),
                                     nameRectObj["h"].toDouble());
                ch.nameFontSize = chObj["nameFontSize"].toInt(20);
                ch.nameColor = QColor(chObj["nameColor"].toString());
                if (!ch.nameColor.isValid()) ch.nameColor = Qt::white;
                ch.nameBgImage = chObj["nameBgImage"].toString();
                slide.characters.append(ch);
            }
            if (slideObj.contains("layerOrder")) {
                QJsonArray orderArray = slideObj["layerOrder"].toArray();
                for (const auto &val : orderArray) {
                    slide.layerOrder.append(val.toString());
                }
            }
            // 读取背景音乐行为（默认 Continue）
            if (slideObj.contains("bgmAction"))
                slide.bgmAction = static_cast<BgmAction>(slideObj["bgmAction"].toInt());
            else
                slide.bgmAction = BgmAction::Continue;  // 兼容旧 JSON

            slide.bgmPath = slideObj["bgmPath"].toString("");
            slide.slideAudioPath = slideObj["slideAudioPath"].toString("");
            node.slides.append(slide);
        }

        // choices
        if (node.type == NodeType::Choice) {
            QJsonArray choicesArray = nodeObj["choices"].toArray();
            for (const auto &chVal : choicesArray) {
                QJsonObject chObj = chVal.toObject();
                NodeData::Choice ch;
                ch.text = chObj["text"].toString();
                ch.imagePath = chObj["image"].toString();
                ch.targetNodeId = chObj["targetId"].toString();
                QJsonObject posObj = chObj["position"].toObject();
                ch.buttonPos = QPointF(posObj["x"].toDouble(), posObj["y"].toDouble());
                ch.textFontSize = chObj["textFontSize"].toInt(20);
                ch.textColor = QColor(chObj["textColor"].toString());
                if (!ch.textColor.isValid()) ch.textColor = Qt::white;
                double w = chObj["buttonWidth"].toDouble(400);
                double h = chObj["buttonHeight"].toDouble(100);
                ch.buttonSize = QSizeF(w, h);
                node.choices.append(ch);
            }
        }
        tempNodes[node.id] = node;
    }

    // 重建 childIds
    for (auto &node : tempNodes) {
        if (!node.parentId.isEmpty() && tempNodes.contains(node.parentId)) {
            tempNodes[node.parentId].childIds.append(node.id);
        }
    }
    m_nodes = tempNodes;
    for (auto &node : tempNodes) {
        if (node.slides.isEmpty()) {
            Slide defaultSlide;
            defaultSlide.textRect = QRectF(1920/2 - 750, 1080 - 450, 1500, 400);
            defaultSlide.textFontSize = 20;
            defaultSlide.textColor = Qt::white;
            node.slides.append(defaultSlide);
        }
    }
    if (!m_rootId.isEmpty() && !m_nodes.contains(m_rootId)) {
        m_rootId.clear();
        for (const auto &node : m_nodes) {
            if (node.parentId.isEmpty()) {
                m_rootId = node.id;
                break;
            }
        }
    }
    emit dataChanged();
    return true;
}