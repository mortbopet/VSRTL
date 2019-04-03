#ifndef VSRTL_GRAPHICS_DEFINES_H
#define VSRTL_GRAPHICS_DEFINES_H

#include <QMetaType>
#include "vsrtl_defines.h"

// Allow vsrtl base value types to be used as a QVariant
Q_DECLARE_METATYPE(vsrtl::VSRTL_VT_S)
Q_DECLARE_METATYPE(vsrtl::VSRTL_VT_U)

namespace vsrtl {

enum class ValueDisplayFormat { binary = 2, baseTen = 10, hex = 16, unicode = 99 };

#define WIRE_DEFAULT_COLOR QColor("#636363")
#define WIRE_SELECTED_COLOR QColor("#FEF160")
#define WIRE_BOOLHIGH_COLOR QColor("#6EEB83")
#define BACKGROUND_COLOR QColor("#222222")
#define BUTTON_COLLAPSE_COLOR QColor("#6bc8ff")
#define BUTTON_EXPAND_COLOR QColor("#26a65b")

#define GRID_SIZE 15
#define CHIP_MARGIN 3

#define TOP_MARGIN 10
#define BOT_MARGIN TOP_MARGIN
#define SIDE_MARGIN TOP_MARGIN
#define RESIZE_CURSOR_MARGIN 10

#define IO_MIN_SPACING TOP_MARGIN / 2
#define IO_PIN_LEN SIDE_MARGIN / 2

#define BUTTON_INDENT 2

#define WIRE_WIDTH 4
#define COMPONENT_BORDER_WIDTH 3

#define PORT_INNER_MARGIN 5

}  // namespace vsrtl
#endif  // VSRTL_GRAPHICS_DEFINES_H
