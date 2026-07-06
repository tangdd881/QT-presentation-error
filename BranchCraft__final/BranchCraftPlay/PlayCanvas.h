#ifndef PLAYCANVAS_H
#define PLAYCANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QPointF>
#include "PlayerStoryGraph.h"

class PlayCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit PlayCanvas(StoryGraph *graph, const QString &projectPath, QWidget *parent = nullptr);

    void setCurrentNode(const QString &nodeId, int slideIndex);
    void setCurrentSlideIndex(int index);

signals:
    void choiceSelected(const QString &targetNodeId);
    void canvasClicked();
    void historyBackRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateFromNode();
    void updateDrawRect();
    QPointF toVirtual(const QPointF &pos) const;
    QPointF toPhysical(const QPointF &virtualPos) const;
    void rebuildDrawOrder();

    // 绘制项
    struct CharItem {
        QString imagePath;
        QPixmap pixmap;
        QPointF pos;            // 图片中心点
        double scale;
        QRectF rect;            // 图片实际矩形
        QString name;
        QRectF nameRect;
        int nameFontSize;
        QColor nameColor;
        QPixmap nameBgPixmap;
    };

    struct TextItem {
        QRectF rect;
        int fontSize;
        QPixmap bgPixmap;
        QString text;
        QColor textColor;
    };

    struct ChoiceItem {
        QString text;
        QPixmap bgPixmap;
        QPointF pos;
        QRectF rect;
        QString targetNodeId;
        int fontSize;
        QColor textColor;
        QSizeF buttonSize;
    };

    struct DrawableItem {
        enum Type { Char, Text, Choice, Name };
        Type type;
        int index;          // 对于 Char/Name/Choice 有效，Text 为 -1
        QString identifier;
    };

    StoryGraph *m_graph;
    QString m_projectPath;
    QString m_currentNodeId;
    int m_currentSlideIndex;
    QSizeF m_stageSize;     // 虚拟舞台大小 1920x1080
    QRectF m_drawRect;      // 实际绘制矩形
    double m_scale;         // 缩放比例

    QPixmap m_background;
    QString m_textContent;
    TextItem m_textItem;
    QList<CharItem> m_characters;
    QList<ChoiceItem> m_choices;
    QList<DrawableItem> m_drawOrder;
};

#endif // PLAYCANVAS_H