#ifndef VSRTL_GRAPHICS_DEFINES_H
#define VSRTL_GRAPHICS_DEFINES_H

#include <QColor>
#include <QMetaType>

#include "../interface/vsrtl_defines.h"

#define DRAW_BOUNDING_RECT(painter)                                            \
  painter->save();                                                             \
  painter->setPen(QPen(Qt::red, 1));                                           \
  painter->setBrush(Qt::transparent);                                          \
  painter->drawRect(boundingRect());                                           \
  painter->restore();

// Allow vsrtl base value types to be used as a QVariant
Q_DECLARE_METATYPE(vsrtl::VSRTL_VT_S)
Q_DECLARE_METATYPE(vsrtl::VSRTL_VT_U)

namespace vsrtl {

enum class ValueDisplayFormat {
  binary = 2,
  baseTen = 10,
  hex = 16,
  unicode = 99
};

constexpr QColor WIRE_DEFAULT_COLOR = {0x63, 0x63, 0x63};
constexpr QColor WIRE_SELECTED_COLOR = {0xFE, 0xF1, 0x60};
constexpr QColor WIRE_BOOLHIGH_COLOR = {0x6E, 0xEB, 0x83};
constexpr QColor WIRE_HIGH_COLOR = {0xFF, 0xD5, 0x2E};
constexpr QColor BACKGROUND_COLOR = {0x22, 0x22, 0x22};
constexpr QColor BUTTON_COLLAPSE_COLOR = {0x6b, 0xc8, 0xff};
constexpr QColor BUTTON_EXPAND_COLOR = {0x26, 0xa6, 0x5b};

#define GRID_SIZE 14

#define TOP_MARGIN 10
#define BOT_MARGIN TOP_MARGIN
#define SIDE_MARGIN TOP_MARGIN
#define RESIZE_CURSOR_MARGIN 10

#define IO_MIN_SPACING TOP_MARGIN / 2
#define IO_PIN_LEN SIDE_MARGIN / 2

#define BUTTON_INDENT 2

#define SUBCOMPONENT_INDENT 3
#define COMPONENT_COLUMN_MARGIN 2
#define COMPONENT_ROW_MARGIN COMPONENT_COLUMN_MARGIN / 2

#define WIRE_WIDTH 2
#define COMPONENT_BORDER_WIDTH 2

#define PORT_INNER_MARGIN 5

} // namespace vsrtl
#endif // VSRTL_GRAPHICS_DEFINES_H
