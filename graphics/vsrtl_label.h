#pragma once

#include "vsrtl_graphicsbaseitem.h"
#include "vsrtl_qt_serializers.h"

#include "cereal/cereal.hpp"

#include <QFont>

namespace vsrtl {

class Label : public GraphicsBaseItem<QGraphicsTextItem> {
  Q_OBJECT
public:
  Label(QGraphicsItem *parent, const QString &text,
        std::shared_ptr<QAction> visibilityAction = {}, int fontSize = 12);

  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item,
             QWidget *) override;
  QPainterPath shape() const override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

  void setHoverable(bool enabled);
  void setText(const QString &text);
  void setAlignment(Qt::Alignment alignment);
  void setPointSize(int size);
  void setLocked(bool locked) override;
  void forceDefaultTextColor(const QColor &color);
  void clearForcedDefaultTextColor() { m_defaultColorOverridden = false; }

  /**
   * @brief updateText
   * Triggers an update of the current text. Relevant if the Label gathers its
   * text from some dynamic structure.
   */
  virtual void updateText();

  template <class Archive>
  void serialize(Archive &archive) {
    prepareGeometryChange();
    try {
      bool v = m_visibilityAction->isChecked();
      archive(cereal::make_nvp("Visible", v));
      m_visibilityAction->setChecked(v);

    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      bool bold = m_font.bold();
      archive(cereal::make_nvp("Bold", bold));
      m_font.setBold(bold);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      bool italic = m_font.italic();
      archive(cereal::make_nvp("Italic", italic));
      m_font.setItalic(italic);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      int ptSize = m_font.pointSize();
      archive(cereal::make_nvp("PtSize", ptSize));
      m_font.setPointSize(ptSize);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      QString text = toPlainText();
      archive(cereal::make_nvp("Text", text));
      setPlainText(text);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      QPointF p = pos();
      archive(cereal::make_nvp("Pos", p));
      setPos(p);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      unsigned alignment = m_alignment;
      archive(cereal::make_nvp("Alignment", alignment));
      m_alignment = static_cast<Qt::Alignment>(alignment);
      setAlignment(m_alignment);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    applyFormatChanges();
  }

protected:
  void applyFormatChanges();
  void editTriggered();

  bool m_hoverable = true;
  QFont m_font;
  bool m_defaultColorOverridden = false;
  Qt::Alignment m_alignment = Qt::AlignCenter;
  std::shared_ptr<QAction> m_visibilityAction;
};

} // namespace vsrtl
