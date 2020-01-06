#pragma once

#include "cereal/cereal.hpp"

#include <QPoint>

// QPoint serializer
template <class Archive>
void serialize(Archive& archive, QPoint& m) {
    archive(cereal::make_nvp("x", m.rx()));
    archive(cereal::make_nvp("y", m.ry()));
}
