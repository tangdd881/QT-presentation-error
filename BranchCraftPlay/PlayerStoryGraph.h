#ifndef PLAYERSTORYGRAPH_H
#define PLAYERSTORYGRAPH_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QColor>

enum class NodeType { Normal, Choice };
enum class BgmAction { Continue, Stop, Switch };

struct Slide {
    QString backgroundImage;
    struct Character {
        QString name;
        QString imagePath;
        QPointF position;
        double scale = 1.0;
        QRectF nameRect;
        int nameFontSize = 20;
        QColor nameColor = Qt::white;
        QString nameBgImage;
    };
    QList<Character> characters;
    QString text;
    QRectF textRect;
    int textFontSize = 20;
    QString textBgImage;
    QColor textColor = Qt::white;
    double duration = 0.0;
    QStringList layerOrder;
    BgmAction bgmAction = BgmAction::Continue;   // 新增
    QString bgmPath;                             // 新增
    QString slideAudioPath;
};

struct NodeData {
    QString id;
    NodeType type;
    QString name;
    QList<Slide> slides;
    QString audioPath;
    struct Choice {
        QString text;
        QString imagePath;
        QString targetNodeId;
        QPointF buttonPos;
        int textFontSize = 20;
        QColor textColor = Qt::white;
        QSizeF buttonSize = QSizeF(400, 100);
    };
    QList<Choice> choices;
    QString parentId;
    QList<QString> childIds;
};

class StoryGraph : public QObject
{
    Q_OBJECT
public:
    explicit StoryGraph(QObject *parent = nullptr);

    // 节点操作（只读+加载用）
    NodeData getNode(const QString &id) const;
    QList<NodeData> getAllNodes() const { return m_nodes.values(); }
    QString getRootId() const { return m_rootId; }
    void clear();

    // 序列化（仅用于加载）
    bool fromJson(const QJsonObject &obj);

signals:
    void dataChanged();

private:
    QMap<QString, NodeData> m_nodes;
    QString m_rootId;
};

#endif // PLAYERSTORYGRAPH_H