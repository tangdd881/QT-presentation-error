#include "PlayCanvas.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>

PlayCanvas::PlayCanvas(StoryGraph *graph, const QString &projectPath, QWidget *parent)
    : QWidget(parent)
    , m_graph(graph)
    , m_projectPath(projectPath)
    , m_currentSlideIndex(0)
    , m_stageSize(1920, 1080)
{
    setStyleSheet("background-color: #2a2a2a;");
    setMouseTracking(true);
}

void PlayCanvas::setCurrentNode(const QString &nodeId, int slideIndex)
{
    if (m_currentNodeId == nodeId && m_currentSlideIndex == slideIndex) {
        updateFromNode();
        update();
        return;
    }
    m_currentNodeId = nodeId;
    m_currentSlideIndex = slideIndex;
    updateFromNode();
    update();
}

void PlayCanvas::setCurrentSlideIndex(int index)
{
    if (m_currentSlideIndex == index) return;
    m_currentSlideIndex = index;
    updateFromNode();
    update();
}

void PlayCanvas::updateFromNode()
{
    m_characters.clear();
    m_choices.clear();
    m_background = QPixmap();
    m_textContent.clear();
    m_textItem = TextItem();

    if (m_currentNodeId.isEmpty()) return;

    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty()) return;

    if (node.slides.isEmpty()) {
        qWarning() << "节点没有幻灯片:" << m_currentNodeId;
        return;
    }

    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= node.slides.size()) {
        m_currentSlideIndex = 0;
    }

    const Slide &slide = node.slides[m_currentSlideIndex];

    // 加载背景
    if (!slide.backgroundImage.isEmpty()) {
        QString fullPath = m_projectPath + "/" + slide.backgroundImage;
        QImageReader reader(fullPath);
        if (reader.canRead()) {
            QSize targetSize = reader.size().scaled(m_stageSize.width(), m_stageSize.height(), Qt::KeepAspectRatio);
            reader.setScaledSize(targetSize);
            QImage image = reader.read();
            if (!image.isNull()) {
                m_background = QPixmap::fromImage(image);
            }
        }
    }

    // 文本内容
    m_textContent = slide.text;

    // 文本框
    m_textItem.rect = slide.textRect;
    m_textItem.fontSize = slide.textFontSize;
    m_textItem.textColor = slide.textColor;
    if (!slide.textBgImage.isEmpty()) {
        QString fullPath = m_projectPath + "/" + slide.textBgImage;
        QImageReader reader(fullPath);
        if (reader.canRead()) {
            QSize targetSize = reader.size().scaled(800, 400, Qt::KeepAspectRatio);
            reader.setScaledSize(targetSize);
            QImage image = reader.read();
            if (!image.isNull()) {
                m_textItem.bgPixmap = QPixmap::fromImage(image);
            }
        }
    }

    // 加载角色
    for (int i = 0; i < slide.characters.size(); ++i) {
        const auto &ch = slide.characters[i];
        QString fullPath = m_projectPath + "/" + ch.imagePath;
        QImageReader reader(fullPath);
        if (!reader.canRead()) {
            qDebug() << "无法读取角色图片:" << fullPath;
            continue;
        }
        QSize targetSize = reader.size().scaled(800, 800, Qt::KeepAspectRatio);
        reader.setScaledSize(targetSize);
        QImage image = reader.read();
        if (image.isNull()) continue;

        CharItem item;
        item.imagePath = ch.imagePath;
        item.pixmap = QPixmap::fromImage(image);
        item.pos = ch.position;
        if (item.pos == QPointF(0, 0)) {
            item.pos = QPointF(m_stageSize.width() / 2, m_stageSize.height() / 2);
        }
        item.scale = ch.scale;
        QSizeF scaledSize = item.pixmap.size() * item.scale;
        item.rect = QRectF(item.pos - QPointF(scaledSize.width() / 2, scaledSize.height() / 2), scaledSize);
        item.name = ch.name;
        item.nameRect = ch.nameRect;
        item.nameFontSize = ch.nameFontSize;
        item.nameColor = ch.nameColor;
        if (!ch.nameBgImage.isEmpty()) {
            QPixmap bg(m_projectPath + "/" + ch.nameBgImage);
            if (!bg.isNull()) item.nameBgPixmap = bg;
        }
        m_characters.append(item);
    }

    // 加载分支选项
    if (node.type == NodeType::Choice) {
        for (const auto &choice : node.choices) {
            ChoiceItem item;
            item.text = choice.text;
            item.pos = choice.buttonPos;
            item.targetNodeId = choice.targetNodeId;
            item.fontSize = choice.textFontSize;
            item.textColor = choice.textColor;
            item.buttonSize = choice.buttonSize;
            if (item.buttonSize.width() <= 0) item.buttonSize.setWidth(400);
            if (item.buttonSize.height() <= 0) item.buttonSize.setHeight(100);
            item.rect = QRectF(item.pos - QPointF(item.buttonSize.width() / 2, item.buttonSize.height() / 2),
                               item.buttonSize);
            if (!choice.imagePath.isEmpty()) {
                QString fullPath = m_projectPath + "/" + choice.imagePath;
                QImageReader reader(fullPath);
                if (reader.canRead()) {
                    QSize targetSize = reader.size().scaled(400, 200, Qt::KeepAspectRatio);
                    reader.setScaledSize(targetSize);
                    QImage image = reader.read();
                    if (!image.isNull()) {
                        item.bgPixmap = QPixmap::fromImage(image);
                    }
                }
            }
            m_choices.append(item);
        }
    }

    rebuildDrawOrder();
}

void PlayCanvas::rebuildDrawOrder()
{
    m_drawOrder.clear();

    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;

    const Slide &slide = node.slides[m_currentSlideIndex];

    // 按图层顺序构建
    if (!slide.layerOrder.isEmpty()) {
        for (const QString &id : slide.layerOrder) {
            if (id == "text") {
                m_drawOrder.append({DrawableItem::Text, -1, "text"});
            } else if (id.startsWith("char_")) {
                int idx = id.mid(5).toInt();
                if (idx >= 0 && idx < m_characters.size()) {
                    m_drawOrder.append({DrawableItem::Char, idx, id});
                }
            } else if (id.startsWith("name_")) {
                int idx = id.mid(5).toInt();
                if (idx >= 0 && idx < m_characters.size()) {
                    m_drawOrder.append({DrawableItem::Name, idx, id});
                }
            } else if (id.startsWith("choice_")) {
                int idx = id.mid(7).toInt();
                if (idx >= 0 && idx < m_choices.size()) {
                    m_drawOrder.append({DrawableItem::Choice, idx, id});
                }
            }
        }
    } else {
        // 默认顺序：先角色，再名字，再文本框，最后选项
        for (int i = 0; i < m_characters.size(); ++i) {
            m_drawOrder.append({DrawableItem::Char, i, QString("char_%1").arg(i)});
            m_drawOrder.append({DrawableItem::Name, i, QString("name_%1").arg(i)});
        }
        m_drawOrder.append({DrawableItem::Text, -1, "text"});
        for (int i = 0; i < m_choices.size(); ++i) {
            m_drawOrder.append({DrawableItem::Choice, i, QString("choice_%1").arg(i)});
        }
    }
}

void PlayCanvas::updateDrawRect()
{
    if (width() <= 0 || height() <= 0) return;
    double widgetAspect = (double)width() / height();
    double stageAspect = m_stageSize.width() / m_stageSize.height();

    if (widgetAspect > stageAspect) {
        int drawHeight = height();
        int drawWidth = drawHeight * stageAspect;
        m_drawRect = QRectF((width() - drawWidth) / 2, 0, drawWidth, drawHeight);
    } else {
        int drawWidth = width();
        int drawHeight = drawWidth / stageAspect;
        m_drawRect = QRectF(0, (height() - drawHeight) / 2, drawWidth, drawHeight);
    }
    m_scale = m_drawRect.width() / m_stageSize.width();
}

QPointF PlayCanvas::toVirtual(const QPointF &pos) const
{
    return (pos - m_drawRect.topLeft()) / m_scale;
}

QPointF PlayCanvas::toPhysical(const QPointF &virtualPos) const
{
    return virtualPos * m_scale + m_drawRect.topLeft();
}

void PlayCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 40, 40));

    if (m_drawRect.isEmpty()) updateDrawRect();

    painter.save();
    painter.setClipRect(m_drawRect);
    painter.translate(m_drawRect.topLeft());
    painter.scale(m_scale, m_scale);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 背景绘制 - 使用三参数重载
    if (!m_background.isNull()) {
        QRectF target(0, 0, m_stageSize.width(), m_stageSize.height());
        painter.drawPixmap(target, m_background, m_background.rect());
    } else {
        painter.fillRect(QRectF(0, 0, m_stageSize.width(), m_stageSize.height()), QColor(60, 60, 60));
    }
    // 按图层顺序绘制
    for (const DrawableItem &item : m_drawOrder) {
        if (item.type == DrawableItem::Char) {
            if (item.index < 0 || item.index >= m_characters.size()) continue;
            const CharItem &ch = m_characters[item.index];
            QSizeF scaledSize = ch.pixmap.size() * ch.scale;
            QRectF targetRect(ch.pos.x() - scaledSize.width() / 2,
                              ch.pos.y() - scaledSize.height() / 2,
                              scaledSize.width(), scaledSize.height());
            painter.drawPixmap(targetRect.toRect(), ch.pixmap);
        }
        else if (item.type == DrawableItem::Name) {
            if (item.index < 0 || item.index >= m_characters.size()) continue;
            const CharItem &ch = m_characters[item.index];
            if (ch.name.isEmpty()) continue;
            painter.save();
            if (!ch.nameBgPixmap.isNull()) {
                painter.drawPixmap(ch.nameRect.toRect(), ch.nameBgPixmap);
            } else {
                painter.fillRect(ch.nameRect, QColor(0, 0, 0, 200));
            }
            QFont font;
            font.setPointSize(ch.nameFontSize);
            painter.setFont(font);
            painter.setPen(ch.nameColor);
            painter.drawText(ch.nameRect, Qt::AlignCenter, ch.name);
            painter.restore();
        }
        else if (item.type == DrawableItem::Text) {
            if (m_textItem.rect.isEmpty()) continue;
            painter.save();
            if (!m_textItem.bgPixmap.isNull()) {
                painter.drawPixmap(m_textItem.rect.toRect(), m_textItem.bgPixmap);
            } else {
                painter.fillRect(m_textItem.rect, QColor(0, 0, 0, 200));
            }
            QFont font;
            font.setPointSize(m_textItem.fontSize);
            painter.setFont(font);
            painter.setPen(m_textItem.textColor);
            painter.drawText(m_textItem.rect, Qt::TextWordWrap, m_textContent);
            painter.restore();
        }
        else if (item.type == DrawableItem::Choice) {
            if (item.index < 0 || item.index >= m_choices.size()) continue;
            const ChoiceItem &choice = m_choices[item.index];
            painter.save();
            if (!choice.bgPixmap.isNull()) {
                painter.drawPixmap(choice.rect.toRect(), choice.bgPixmap);
            } else {
                painter.fillRect(choice.rect, QColor(0, 120, 215));
            }
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRect(choice.rect);
            QFont font;
            font.setPointSize(choice.fontSize);
            painter.setFont(font);
            painter.setPen(choice.textColor);
            painter.drawText(choice.rect, Qt::AlignCenter, choice.text);
            painter.restore();
        }
    }

    painter.restore();
}

void PlayCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDrawRect();
}

void PlayCanvas::mousePressEvent(QMouseEvent *event)
{
    // 右键：请求返回上一节点
    if (event->button() == Qt::RightButton) {
        emit historyBackRequested();
        event->accept();
        return;
    }

    // 左键原有逻辑
    NodeData node = m_graph->getNode(m_currentNodeId);

    if (node.type != NodeType::Choice) {
        emit canvasClicked();
        event->accept();
        return;
    }

    // 分支节点：检测是否点击了选项按钮
    QPointF virtualPos = toVirtual(event->pos());
    for (int i = m_drawOrder.size() - 1; i >= 0; --i) {
        const DrawableItem &item = m_drawOrder[i];
        if (item.type == DrawableItem::Choice) {
            if (item.index < 0 || item.index >= m_choices.size()) continue;
            if (m_choices[item.index].rect.contains(virtualPos)) {
                emit choiceSelected(m_choices[item.index].targetNodeId);
                event->accept();
                return;
            }
        }
    }

    event->accept();
}