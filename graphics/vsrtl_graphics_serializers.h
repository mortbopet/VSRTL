#pragma once

#include "cereal/cereal.hpp"

#include <QPoint>
#include <QRect>

// QPoint serializer
template <class Archive>
void serialize(Archive& archive, QPoint& m) {
    archive(cereal::make_nvp("x", m.rx()));
    archive(cereal::make_nvp("y", m.ry()));
}

// QRect serializer
template <class Archive>
void serialize(Archive& archive, QRect& m) {
    int x = m.x();
    int y = m.y();
    int w = m.width();
    int h = m.height();
    archive(cereal::make_nvp("x", x));
    archive(cereal::make_nvp("y", y));
    archive(cereal::make_nvp("width", w));
    archive(cereal::make_nvp("height", h));
    m.setX(x);
    m.setY(y);
    m.setHeight(h);
    m.setWidth(w);
}
