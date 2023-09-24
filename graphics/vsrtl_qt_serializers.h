#pragma once

#include "cereal/cereal.hpp"

#include <QPoint>
#include <QRect>
#include <QString>

// QPoint serializer
template <class Archive>
void serialize(Archive &archive, QPoint &m) {
  archive(cereal::make_nvp("x", m.rx()));
  archive(cereal::make_nvp("y", m.ry()));
}

template <class Archive>
void serialize(Archive &archive, QPointF &m) {
  archive(cereal::make_nvp("x", m.rx()));
  archive(cereal::make_nvp("y", m.ry()));
}

// QRect serializer
template <class Archive>
void serialize(Archive &archive, QRect &m) {
  int x = m.x();
  int y = m.y();
  int w = m.width();
  int h = m.height();
  archive(cereal::make_nvp("x", x));
  archive(cereal::make_nvp("y", y));
  archive(cereal::make_nvp("w", w));
  archive(cereal::make_nvp("h", h));
  m.setX(x);
  m.setY(y);
  m.setHeight(h);
  m.setWidth(w);
}

// QString serializer
template <class Archive>
void serialize(Archive &archive, QString &m) {
  std::string str = m.toStdString();
  archive(cereal::make_nvp("str", str));
  m = QString::fromStdString(str);
}
