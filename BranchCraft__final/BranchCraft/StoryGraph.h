#ifndef STORYGRAPH_H
#define STORYGRAPH_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <QPointF>
#include <QUuid>
#include <QRectF>
#include <QColor>
#include <QRecursiveMutex>

enum class NodeType { Normal, Choice };
enum class BgmAction { Continue, Stop, Switch };
struct Slide {
    QString backgroundImage;
    struct Character {
        QString name;               // 角色名字内容
        QString imagePath;          // 角色图片路径
        QPointF position;           // 角色图片中心点（虚拟坐标）
        double scale = 1.0;         // 角色图片缩放

        // 角色名字的独立文本属性
        QRectF nameRect;            // 名字文本框区域（虚拟坐标）
        int nameFontSize = 20;      // 名字字体大小
        QColor nameColor ; // 名字文字颜色
        QString nameBgImage;        // 名字文本框背景图（相对路径）
    };
    QList<Character> characters;
    QString text;
    QRectF textRect;           // 文本框区域（虚拟坐标）
    int textFontSize = 20;     // 文字大小（像素）
    QString textBgImage;
    QColor textColor ;
    double duration = 0.0;
    QStringList layerOrder;
    //音频相关
    BgmAction bgmAction = BgmAction::Continue;   // 背景音乐行为
    QString bgmPath;                             // 当 bgmAction == Switch 时使用的新 BGM 路径
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
        QString imagePath;          // 按钮背景图片
        QString targetNodeId;
        QPointF buttonPos;
        int textFontSize = 20;      // 按钮文字大小
        QColor textColor;  // 按钮文字颜色
        QSizeF buttonSize = QSizeF(400, 100);  // 新增：按钮大小（宽, 高）
    };
    QList<Choice> choices;
    // 树形结构字段
    QString parentId;           // 父节点ID，根节点为空
    QList<QString> childIds;    // 子节点ID列表（按顺序）
};

class StoryGraph : public QObject
{
    Q_OBJECT
public:
    explicit StoryGraph(QObject *parent = nullptr);

    // 节点操作
    void addNode(const NodeData &node, const QString &parentId = QString());
    void removeNode(const QString &id);
    void updateNode(const NodeData &node);
    void moveNode(const QString &id, const QString &newParentId, int newIndex = -1);
    NodeData getNode(const QString &id) const;
    QList<NodeData> getAllNodes() const { return m_nodes.values(); }
    QList<NodeData> getChildren(const QString &parentId) const;
    QString getRootId() const { return m_rootId; }
    void setRootId(const QString &id);
    void clear();

    // 序列化
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &obj);

signals:
    void dataChanged();

private:
    QMap<QString, NodeData> m_nodes;
    QString m_rootId;
private:
    mutable QRecursiveMutex m_mutex;  // 递归锁，防止同一线程死锁

    void rebuildChildIds();      // 根据每个节点的 parentId 重建 childIds 列表（保证一致性）
    void removeSubtree(const QString &id);
};

#endif // STORYGRAPH_H