#ifndef SCENECANVAS_H
#define SCENECANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QPointF>
#include "StoryGraph.h"

class SceneCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit SceneCanvas(StoryGraph *graph, QWidget *parent = nullptr);

    // 设置要显示的节点
    void setCurrentNode(const QString &nodeId);
    //设置显示的幻灯片：
    void setCurrentSlideIndex(int index);
    void setProjectPath(const QString &path) { m_projectPath = path; }
    void updateFromNode();
    void refreshDisplay();

signals:
    void nodeDataChanged(const QString &nodeId);
    void characterNameRectChanged(int charIndex, const QRectF &rect);
    void textRectChanged(const QRectF &rect);   // 可选，用于文本框拖拽
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public:
    QString m_projectPath;//工程路径             // 从 StoryGraph 刷新数据
    void saveItemPositions();            // 保存拖拽后的位置到 StoryGraph
    void updateDrawRect();
    void loadTextItem();
    void saveTextItemPosition();
    void saveCharacterNameRect(int index);
    void rebuildDrawOrder();
    void bringToFront(const QString &identifier);
    QPointF toVirtual(const QPointF &pos)const;
    QPointF toPhysical(const QPointF &virtualPos)const;
    StoryGraph *m_graph;
    QString m_currentNodeId;
    int m_currentSlideIndex;
    QSizeF m_stageSize;  // 虚拟舞台大小，例如 1920x1080
    QRectF m_drawRect;   // 实际绘制矩形（用于坐标映射）
    double m_scale;      // 实际像素到虚拟坐标的缩放比

    // 角色项
    struct CharItem {
        QString imagePath;
        QPixmap pixmap;
        QPointF pos;            // 图片中心点
        double scale;
        QRectF rect;            // 图片实际矩形

        // 名字独立属性
        QString name;
        QRectF nameRect;        // 名字文本框区域（虚拟坐标）
        int nameFontSize;
        QColor nameColor;
        QPixmap nameBgPixmap;
        QString nameBgImagePath;
    };
    QList<CharItem> m_characters;

    // 选项按钮项（仅分支节点）
    struct ChoiceItem {
        QString text;
        QPixmap bgPixmap;           // 背景图片
        QPointF pos;
        QRectF rect;
        QString targetNodeId;
        int fontSize = 20;
        QColor textColor = Qt::white;
        QSizeF buttonSize;
    };
    QList<ChoiceItem> m_choices;

    QPixmap m_background;

    //文本框项
    QString m_text;
    struct TextItem {
        QRectF rect;
        int fontSize;
        QString bgImagePath;
        QPixmap bgPixmap;
        QColor textColor;
    };
    TextItem m_textItem;

    struct DrawableItem {
        enum Type { Char, Text, Choice, Name };  // 新增 Name
        Type type;
        int index;          // 对于 Char/Name/Choice 有效，-1 表示 Text
        QString identifier; // 如 "char_0", "name_0", "text", "choice_0"
        QRectF rect;
    };
    QList<DrawableItem> m_drawOrder;

    // 拖拽状态
    enum DragType { None, Character, Choice, Text,CharacterName };
    DragType m_dragType;
    int m_dragIndex;
    QPointF m_dragStartPos;
};

#endif // SCENECANVAS_H