#include "vsrtl_label.h"

#include <QFontMetrics>
#include <QGraphicsSceneContextMenuEvent>
#include <QInputDialog>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "vsrtl_labeleditdialog.h"
#include "vsrtl_scene.h"

namespace vsrtl {

Label::Label(QString text, QGraphicsItem* parent, int fontSize) : GraphicsBase(parent) {
    m_font = QFont("Monospace", fontSize);

    setText(text);
    setFlags(ItemIsSelectable);
    setMoveable();
}

QRectF Label::boundingRect() const {
    return m_textRect.translated(-m_textRect.width() / 2, -m_textRect.height() / 2);
}

void Label::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (isLocked())
        return;

    QMenu menu;
    auto* editAction = menu.addAction("Edit label");
    connect(editAction, &QAction::triggered, this, &Label::editTriggered);

    auto* hideAction = menu.addAction("Hide label");
    connect(hideAction, &QAction::triggered, [=] { setVisible(false); });

    menu.exec(event->screenPos());
};

void Label::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    editTriggered();
}

void Label::editTriggered() {
    LabelEditDialog diag;
    diag.m_ui->bold->setChecked(m_font.bold());
    diag.m_ui->italic->setChecked(m_font.italic());
    diag.m_ui->size->setValue(m_font.pointSize());
    diag.m_ui->text->setText(m_text);

    if (diag.exec()) {
        m_font.setBold(diag.m_ui->bold->isChecked());
        m_font.setItalic(diag.m_ui->italic->isChecked());
        m_font.setPointSize(diag.m_ui->size->value());
        setText(diag.m_ui->text->text());
    }
}

void Label::setText(const QString& text) {
    prepareGeometryChange();
    QFontMetricsF fm(m_font);
    m_text = text;
    m_textRect = fm.boundingRect(m_text);
}

void Label::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // Draw text
    if (lod >= 0.4) {
        QPointF offset(-m_textRect.width() / 2, -m_textRect.height() / 2);
        painter->setFont(m_font);
        painter->save();
        if (static_cast<VSRTLScene*>(scene())->darkmode()) {
            auto pen = painter->pen();
            pen.setColor(QColor(Qt::lightGray).lighter());
            painter->setPen(pen);
        }
        painter->drawText(offset, m_text);
        painter->restore();
    }
}

}  // namespace vsrtl
