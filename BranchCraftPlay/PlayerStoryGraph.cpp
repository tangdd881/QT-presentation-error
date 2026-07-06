#include "PlayerStoryGraph.h"
#include <QDebug>

StoryGraph::StoryGraph(QObject *parent) : QObject(parent) {}

NodeData StoryGraph::getNode(const QString &id) const
{
    return m_nodes.value(id);
}

void StoryGraph::clear()
{
    m_nodes.clear();
    m_rootId.clear();
    emit dataChanged();
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
        QString typeStr = nodeObj["type"].toString();
        node.type = (typeStr == "choice") ? NodeType::Choice : NodeType::Normal;
        node.name = nodeObj["name"].toString();
        node.audioPath = nodeObj["audio"].toString();
        node.parentId = nodeObj["parentId"].toString();

        // 解析 slides
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

            // 解析角色
            QJsonArray charsArray = slideObj["characters"].toArray();
            for (const auto &chVal : charsArray) {
                QJsonObject chObj = chVal.toObject();
                Slide::Character ch;
                ch.name = chObj["name"].toString();
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

            // 图层顺序
            if (slideObj.contains("layerOrder")) {
                QJsonArray orderArray = slideObj["layerOrder"].toArray();
                for (const auto &idVal : orderArray) {
                    slide.layerOrder.append(idVal.toString());
                }
            }
            //解析音频
            if (slideObj.contains("bgmAction"))
                slide.bgmAction = static_cast<BgmAction>(slideObj["bgmAction"].toInt());
            else
                slide.bgmAction = BgmAction::Continue;

            slide.bgmPath = slideObj["bgmPath"].toString("");
            slide.slideAudioPath = slideObj["slideAudioPath"].toString("");
            node.slides.append(slide);
        }

        // 解析分支选项
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
                ch.buttonSize = QSizeF(chObj["buttonWidth"].toDouble(400),
                                       chObj["buttonHeight"].toDouble(100));
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

    // 确保根节点有效
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