#include "SceneCanvas.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFileInfo>
#include <QDebug>
#include <QImageReader>

SceneCanvas::SceneCanvas(StoryGraph *graph, QWidget *parent)
    : QWidget(parent), m_graph(graph), m_dragType(None), m_dragIndex(-1), m_stageSize(1920, 1080)
{
    setMinimumHeight(400);
    setStyleSheet("background-color: #2a2a2a;");
    setMouseTracking(true);
}

void SceneCanvas::setCurrentNode(const QString &nodeId)
{
    if (m_currentNodeId == nodeId && !m_currentNodeId.isEmpty()) {
        // 相同节点但数据可能已变，仍需重新加载
        updateFromNode();
        update();
        return;
    }
    m_currentNodeId = nodeId;
    m_currentSlideIndex = 0;
    updateFromNode();
    update();
}

void SceneCanvas::setCurrentSlideIndex(int index)
{
    if (m_currentSlideIndex == index) return;
    m_currentSlideIndex = index;
    updateFromNode();
    update();
}


void SceneCanvas::updateFromNode()
{
    m_characters.clear();
    m_choices.clear();
    m_background = QPixmap();
    m_text.clear();

    if (m_currentNodeId.isEmpty())
        return;

    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty())
        return;

    // 确保幻灯片存在且索引有效
    if (node.slides.isEmpty()) {
        qWarning() << "Node" << m_currentNodeId << "has no slides, cannot display";
        return;
    }
    if (m_currentSlideIndex >= node.slides.size())
        m_currentSlideIndex = node.slides.size() - 1;
    if (m_currentSlideIndex < 0)
        m_currentSlideIndex = 0;

    Slide &slide = node.slides[m_currentSlideIndex];
        if (!slide.backgroundImage.isEmpty()) {
            QString fullPath = m_projectPath + "/" + slide.backgroundImage;
            QImageReader reader(fullPath);
            if (reader.canRead()) {
                // 限制背景最大尺寸为舞台大小（1920x1080），防止过大
                QSize targetSize = reader.size().scaled(m_stageSize.width(), m_stageSize.height(), Qt::KeepAspectRatio);
                reader.setScaledSize(targetSize);
                QImage image = reader.read();
                if (!image.isNull()) {
                    m_background = QPixmap::fromImage(image);
                }
            }
        }
        m_text = slide.text;

        // 加载角色
        for (int i = 0; i < slide.characters.size(); ++i) {
            auto &ch = slide.characters[i];
            QString fullPath = m_projectPath + "/" + ch.imagePath;
            QImageReader reader(fullPath);
            if (!reader.canRead()) {
                qDebug() << "Failed to read image:" << fullPath;
                continue;
            }
            // 限制角色图片最大尺寸为 800x800（可根据需要调整）
            QSize targetSize = reader.size().scaled(800, 800, Qt::KeepAspectRatio);
            reader.setScaledSize(targetSize);
            QImage image = reader.read();
            if (image.isNull()) {
                qDebug() << "Failed to decode image:" << reader.errorString();
                continue;
            }
            QPixmap pix = QPixmap::fromImage(image);
            CharItem item;
            item.imagePath = ch.imagePath;
            item.pixmap = pix;
            item.pos = ch.position;
            if (item.pos == QPointF(0, 0)) {
                item.pos = QPointF(m_stageSize.width()/2, m_stageSize.height()/2);
            }
            item.scale = ch.scale;
            QSizeF scaledSize = pix.size() * item.scale;
            item.rect = QRectF(item.pos - QPointF(scaledSize.width()/2, scaledSize.height()/2), scaledSize);
            item.name = ch.name;
            item.nameRect = ch.nameRect;
            item.nameFontSize = ch.nameFontSize;
            item.nameColor = ch.nameColor;
            item.nameBgImagePath = ch.nameBgImage;
            if (!item.nameBgImagePath.isEmpty()) {
                QPixmap bg(m_projectPath + "/" + item.nameBgImagePath);
                if (!bg.isNull()) item.nameBgPixmap = bg;
            }
            m_characters.append(item);
        }

        // 加载文本框
        loadTextItem();

    // 分支节点选项（必须在角色和文本框加载之后，且在 rebuildDrawOrder 之前）
    if (node.type == NodeType::Choice) {
        for (const auto &choice : node.choices) {
            ChoiceItem item;
            item.text = choice.text;
            item.pos = choice.buttonPos.isNull() ? QPointF(100, 100) : choice.buttonPos;
            item.targetNodeId = choice.targetNodeId;
            if (!choice.imagePath.isEmpty()) {
                QString fullPath = m_projectPath + "/" + choice.imagePath;
                QImageReader reader(fullPath);
                if (reader.canRead()) {
                    // 限制按钮图片最大尺寸为 400x200（可根据按钮大小调整）
                    QSize targetSize = reader.size().scaled(400, 200, Qt::KeepAspectRatio);
                    reader.setScaledSize(targetSize);
                    QImage image = reader.read();
                    if (!image.isNull()) {
                        item.bgPixmap = QPixmap::fromImage(image);
                    }
                }
            }
            item.fontSize = choice.textFontSize;
            item.textColor = choice.textColor;
            item.buttonSize = choice.buttonSize;
            QSizeF size = item.buttonSize;
            item.rect = QRectF(item.pos - QPointF(size.width()/2, size.height()/2), size);
            m_choices.append(item);
        }
    }

    // ★ 在所有数据加载完成后重建绘制顺序
    rebuildDrawOrder();
}
void SceneCanvas::updateDrawRect()
{
    if (width() <= 0 || height() <= 0) return;
    double widgetAspect = (double)width() / height();
    double stageAspect = m_stageSize.width() / m_stageSize.height();
    if (widgetAspect > stageAspect) {
        // 宽度过大，高度填满
        int drawHeight = height();
        int drawWidth = drawHeight * stageAspect;
        m_drawRect = QRectF((width() - drawWidth) / 2, 0, drawWidth, drawHeight);
    } else {
        // 高度过大，宽度填满
        int drawWidth = width();
        int drawHeight = drawWidth / stageAspect;
        m_drawRect = QRectF(0, (height() - drawHeight) / 2, drawWidth, drawHeight);
    }
    m_scale = m_drawRect.width() / m_stageSize.width();
}

QPointF SceneCanvas::toVirtual(const QPointF &pos) const
{
    return (pos - m_drawRect.topLeft()) / m_scale;
}

QPointF SceneCanvas::toPhysical(const QPointF &virtualPos) const
{
    return virtualPos * m_scale + m_drawRect.topLeft();
}

void SceneCanvas::paintEvent(QPaintEvent *event)
{
    if (m_currentNodeId.isEmpty() || !m_graph) {
        // 显示空白或提示
        QPainter painter(this);
        painter.fillRect(rect(), QColor(40,40,40));
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("暂无剧情节点"));
        return;
    }
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty()) {
        // 节点已删除，清除当前节点 ID
        m_currentNodeId.clear();
        update();
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 40, 40)); // 外边距黑色

    if (m_drawRect.isEmpty()) updateDrawRect();
    painter.save();
    painter.setClipRect(m_drawRect);
    painter.translate(m_drawRect.topLeft());
    painter.scale(m_scale, m_scale);

    // 1. 绘制背景（背景始终在最底层，不参与图层顺序）
    if (!m_background.isNull()) {
        painter.drawPixmap(QRectF(0, 0, m_stageSize.width(), m_stageSize.height()), m_background, m_background.rect());
    } else {
        painter.fillRect(QRectF(0, 0, m_stageSize.width(), m_stageSize.height()), QColor(60, 60, 60));
        painter.setPen(Qt::lightGray);
        painter.drawText(QRectF(0, 0, m_stageSize.width(), m_stageSize.height()), Qt::AlignCenter, tr("无背景图片"));
    }

    // 2. 按图层顺序绘制所有控件（角色、文本框、选项按钮）
    for (const DrawableItem &item : m_drawOrder) {
        if (item.type == DrawableItem::Char) {
            // 确保索引有效
            if (item.index < 0 || item.index >= m_characters.size()) continue;
            const CharItem &ch = m_characters[item.index];
            QSizeF scaledSize = ch.pixmap.size() * ch.scale;
            QRectF targetRect(ch.pos.x() - scaledSize.width() / 2,
                              ch.pos.y() - scaledSize.height() / 2,
                              scaledSize.width(), scaledSize.height());
            painter.drawPixmap(targetRect.toRect(), ch.pixmap);
            // 调试框（可选）
            painter.setPen(Qt::red);
            painter.drawRect(targetRect);
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
            painter.drawText(m_textItem.rect, Qt::TextWordWrap, m_text);
            painter.restore();
        }
        else if (item.type == DrawableItem::Choice) {
            if (item.index < 0 || item.index >= m_choices.size()) continue;
            const ChoiceItem &choice = m_choices[item.index];
            if (!choice.bgPixmap.isNull()) {
                painter.drawPixmap(choice.rect.toRect(), choice.bgPixmap);
            } else {
                painter.fillRect(choice.rect, QColor(0, 120, 215));
            }
            painter.setPen(Qt::black);
            painter.drawRect(choice.rect);
            QFont font;
            font.setPointSize(choice.fontSize);
            painter.setFont(font);
            painter.setPen(choice.textColor);
            painter.drawText(choice.rect, Qt::AlignCenter, choice.text);
        }
        else if (item.type == DrawableItem::Name) {
            if (item.index < 0 || item.index >= m_characters.size()) continue;
            const CharItem &ch = m_characters[item.index];
            if (ch.name.isEmpty()) continue;

            painter.save();
            // 绘制背景
            if (!ch.nameBgPixmap.isNull()) {
                painter.drawPixmap(ch.nameRect.toRect(), ch.nameBgPixmap);
            } else {
                painter.fillRect(ch.nameRect, QColor(0, 0, 0, 200));
            }
            // 绘制文字
            QFont font;
            font.setPointSize(ch.nameFontSize);
            painter.setFont(font);
            painter.setPen(ch.nameColor);
            painter.drawText(ch.nameRect, Qt::AlignCenter, ch.name);
            painter.restore();
        }
    }

    painter.restore();
}

void SceneCanvas::mousePressEvent(QMouseEvent *event)
{
    QPointF virtualPos = toVirtual(event->pos());

    for (int i = m_drawOrder.size() - 1; i >= 0; --i) {
        const DrawableItem &item = m_drawOrder[i];
        if (item.type == DrawableItem::Char) {
            if (item.index < 0 || item.index >= m_characters.size()) continue;
            if (m_characters[item.index].rect.contains(virtualPos)) {
                QString identifier = item.identifier;
                int idx = item.index;
                bringToFront(identifier);
                m_dragType = Character;
                m_dragIndex = idx;
                m_dragStartPos = virtualPos;
                event->accept();
                return;
            }
        }
        else if (item.type == DrawableItem::Name) {
            if (item.index < 0 || item.index >= m_characters.size()) continue;
            if (m_characters[item.index].nameRect.contains(virtualPos)) {
                QString identifier = item.identifier;
                int idx = item.index;
                bringToFront(identifier);
                m_dragType = CharacterName;
                m_dragIndex = idx;
                m_dragStartPos = virtualPos;
                event->accept();
                return;
            }
        }
        else if (item.type == DrawableItem::Text) {
            if (m_textItem.rect.contains(virtualPos)) {
                bringToFront("text");
                m_dragType = Text;
                m_dragStartPos = virtualPos;
                event->accept();
                return;
            }
        }
        else if (item.type == DrawableItem::Choice) {
            if (item.index < 0 || item.index >= m_choices.size()) continue;
            if (m_choices[item.index].rect.contains(virtualPos)) {
                QString identifier = item.identifier;
                int idx = item.index;
                bringToFront(identifier);
                m_dragType = Choice;
                m_dragIndex = idx;
                m_dragStartPos = virtualPos;
                event->accept();
                return;
            }
        }
    }

    QWidget::mousePressEvent(event);
}
void SceneCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragType == Text) {
        saveTextItemPosition();   // 内部会更新 graph 并发射 nodeDataChanged
        // 额外通知属性面板更新矩形数值
        if (!m_currentNodeId.isEmpty() && m_currentSlideIndex >= 0) {
            NodeData node = m_graph->getNode(m_currentNodeId);
            if (!node.id.isEmpty() && !node.slides.isEmpty()) {
                QRectF rect = node.slides[m_currentSlideIndex].textRect;
                emit textRectChanged(rect);
            }
        }
        m_dragType = None;
        update();
        event->accept();
        return;
    }
    if (m_dragType == CharacterName && m_dragIndex >= 0) {
        QRectF newRect = m_characters[m_dragIndex].nameRect;
        if (!m_currentNodeId.isEmpty() && m_currentSlideIndex >= 0) {
            NodeData node = m_graph->getNode(m_currentNodeId);
            if (!node.id.isEmpty() && !node.slides.isEmpty() &&
                m_dragIndex < node.slides[m_currentSlideIndex].characters.size()) {
                node.slides[m_currentSlideIndex].characters[m_dragIndex].nameRect = newRect;
                m_graph->updateNode(node);
                emit characterNameRectChanged(m_dragIndex, newRect);
            }
        }
        m_dragType = None;
        m_dragIndex = -1;
        update();
        event->accept();
        return;
    }
    if (m_dragType != None && m_dragIndex >= 0) { // Character or Choice
        saveItemPositions();   // 此函数会发射 nodeDataChanged
        m_dragType = None;
        m_dragIndex = -1;
        update();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
void SceneCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragType == Text && m_dragIndex < 0) {
        QPointF delta = toVirtual(event->pos()) - m_dragStartPos;
        m_textItem.rect.translate(delta);
        m_dragStartPos = toVirtual(event->pos());
        update();
        event->accept();
        return;
    }
    if ((m_dragType == Character || m_dragType == Choice) && m_dragIndex >= 0) {
        QPointF newVirtualPos = toVirtual(event->pos());
        QPointF delta = newVirtualPos - m_dragStartPos;
        if (m_dragType == Character) {
            m_characters[m_dragIndex].pos += delta;
            m_characters[m_dragIndex].rect.translate(delta);
        } else {
            m_choices[m_dragIndex].pos += delta;
            m_choices[m_dragIndex].rect.translate(delta);
        }
        m_dragStartPos = newVirtualPos;
        update();
        event->accept();
        return;
    }
    if (m_dragType == CharacterName && m_dragIndex >= 0) {
        QPointF delta = toVirtual(event->pos()) - m_dragStartPos;
        m_characters[m_dragIndex].nameRect.translate(delta);
        m_dragStartPos = toVirtual(event->pos());
        update();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}


void SceneCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDrawRect();
}

void SceneCanvas::saveItemPositions()
{
    if (m_currentNodeId.isEmpty())
        return;
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty())
        return;
    if (!node.slides.isEmpty() && m_currentSlideIndex >= 0 && m_currentSlideIndex < node.slides.size()) {
        Slide &slide = node.slides[m_currentSlideIndex];
        int count = qMin(m_characters.size(), slide.characters.size());
        for (int i = 0; i < count; ++i) {
            slide.characters[i].position = m_characters[i].pos;
        }
        if (node.type == NodeType::Choice) {
            int choiceCount = qMin(m_choices.size(), node.choices.size());
            for (int i = 0; i < choiceCount; ++i) {
                node.choices[i].buttonPos = m_choices[i].pos;
            }
        }
        m_graph->updateNode(node);
        emit nodeDataChanged(m_currentNodeId);
    }
}

void SceneCanvas::saveCharacterNameRect(int index)
{
    if (m_currentNodeId.isEmpty() || m_currentSlideIndex < 0) return;
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;
    if (index < 0 || index >= node.slides[m_currentSlideIndex].characters.size()) return;

    // 更新数据模型中的名字矩形
    node.slides[m_currentSlideIndex].characters[index].nameRect = m_characters[index].nameRect;
    m_graph->updateNode(node);

    // 强制刷新画布
    update();

    // 发出信号，但不触发属性面板的完整重载（我们稍后会修改 MainWindow 的连接）
    emit nodeDataChanged(m_currentNodeId);
}

void SceneCanvas::saveTextItemPosition()
{
    if (m_currentNodeId.isEmpty() || m_currentSlideIndex < 0) return;
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;
    node.slides[m_currentSlideIndex].textRect = m_textItem.rect;
    m_graph->updateNode(node);
    emit nodeDataChanged(m_currentNodeId);
}
void SceneCanvas::loadTextItem()
{
    if (m_currentNodeId.isEmpty() || m_currentSlideIndex < 0) return;
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;
    const Slide &slide = node.slides[m_currentSlideIndex];
    m_textItem.rect = slide.textRect;
    m_textItem.fontSize = slide.textFontSize;
    m_textItem.bgImagePath = slide.textBgImage;
    m_textItem.textColor = slide.textColor;

    // 加载背景图片
    if (!m_textItem.bgImagePath.isEmpty()) {
        QString fullPath = m_projectPath + "/" + m_textItem.bgImagePath;
        QImageReader reader(fullPath);
        if (reader.canRead()) {
            // 限制文本框背景最大尺寸为 800x400
            QSize targetSize = reader.size().scaled(800, 400, Qt::KeepAspectRatio);
            reader.setScaledSize(targetSize);
            QImage image = reader.read();
            if (!image.isNull()) {
                m_textItem.bgPixmap = QPixmap::fromImage(image);
            } else {
                m_textItem.bgPixmap = QPixmap();
            }
        } else {
            m_textItem.bgPixmap = QPixmap();
        }
    } else {
        m_textItem.bgPixmap = QPixmap();
    }

    // 默认位置：画面下方居中
    if (m_textItem.rect.isNull() || m_textItem.rect.isEmpty()) {
        m_textItem.rect = QRectF(m_stageSize.width()/2 - 400,
                                 m_stageSize.height() - 150,
                                 800, 100);
    }
}
//------图层顺序函数-------
void SceneCanvas::rebuildDrawOrder()
{
    m_drawOrder.clear();
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;

    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= node.slides.size()) {
        m_currentSlideIndex = 0;
        if (node.slides.isEmpty()) return;
    }

    Slide &slide = node.slides[m_currentSlideIndex];
    bool needUpdate = false;

    // 1. 收集当前实际存在的所有标识符
    QStringList actualIdentifiers;
    for (int i = 0; i < slide.characters.size(); ++i) {
        actualIdentifiers.append(QString("char_%1").arg(i));
        actualIdentifiers.append(QString("name_%1").arg(i));   // 名字独立标识符
    }
    actualIdentifiers.append("text");
    if (node.type == NodeType::Choice) {
        for (int i = 0; i < node.choices.size(); ++i) {
            actualIdentifiers.append(QString("choice_%1").arg(i));
        }
    }

    // 2. 准备最终绘制顺序
    QStringList finalOrder;

    if (slide.layerOrder.isEmpty()) {
        finalOrder = actualIdentifiers;
        needUpdate = true;
    } else {
        // 先按已有 layerOrder 添加有效项
        for (const QString &id : slide.layerOrder) {
            if (id == "text") {
                m_drawOrder.append({DrawableItem::Text, -1, "text"});
                finalOrder.append("text");
            } else if (id.startsWith("char_")) {
                int idx = id.mid(5).toInt();
                if (idx >= 0 && idx < slide.characters.size()) {
                    m_drawOrder.append({DrawableItem::Char, idx, id});
                    finalOrder.append(id);
                }
            } else if (id.startsWith("name_")) {
                int idx = id.mid(5).toInt();
                if (idx >= 0 && idx < slide.characters.size()) {
                    m_drawOrder.append({DrawableItem::Name, idx, id});
                    finalOrder.append(id);
                }
            } else if (id.startsWith("choice_")) {
                int idx = id.mid(7).toInt();
                if (node.type == NodeType::Choice && idx >= 0 && idx < node.choices.size()) {
                    m_drawOrder.append({DrawableItem::Choice, idx, id});
                    finalOrder.append(id);
                }
            }
        }

        // 3. 补全缺失项（新添加的角色或选项）
        for (const QString &id : actualIdentifiers) {
            if (!finalOrder.contains(id)) {
                if (id == "text") {
                    m_drawOrder.append({DrawableItem::Text, -1, "text"});
                } else if (id.startsWith("char_")) {
                    int idx = id.mid(5).toInt();
                    if (idx >= 0 && idx < slide.characters.size())
                        m_drawOrder.append({DrawableItem::Char, idx, id});
                } else if (id.startsWith("name_")) {
                    int idx = id.mid(5).toInt();
                    if (idx >= 0 && idx < slide.characters.size())
                        m_drawOrder.append({DrawableItem::Name, idx, id});
                } else if (id.startsWith("choice_")) {
                    int idx = id.mid(7).toInt();
                    if (node.type == NodeType::Choice && idx >= 0 && idx < node.choices.size())
                        m_drawOrder.append({DrawableItem::Choice, idx, id});
                }
                finalOrder.append(id);
                needUpdate = true;
            }
        }
    }

    if (needUpdate) {
        slide.layerOrder = finalOrder;
        m_graph->updateNode(node);
    }
}
void SceneCanvas::bringToFront(const QString &identifier)
{
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;
    Slide &slide = node.slides[m_currentSlideIndex];

    // 从 layerOrder 中移除该 identifier 并追加到末尾（最上层）
    slide.layerOrder.removeAll(identifier);
    slide.layerOrder.append(identifier);
    m_graph->updateNode(node);

    // 重新构建绘制顺序并刷新
    rebuildDrawOrder();
    update();
}

void SceneCanvas::refreshDisplay()
{
    // 重新从 StoryGraph 加载当前节点和幻灯片的数据
    updateFromNode();
    // 强制重绘
    update();
}