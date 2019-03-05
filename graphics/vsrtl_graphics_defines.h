#ifndef VSRTL_GRAPHICS_DEFINES_H
#define VSRTL_GRAPHICS_DEFINES_H

#include <QMetaType>
#include "vsrtl_defines.h"

// Allow vsrtl base value types to be used as a QVariant
Q_DECLARE_METATYPE(vsrtl::VSRTL_VT_S)
Q_DECLARE_METATYPE(vsrtl::VSRTL_VT_U)

namespace vsrtl {

enum class ValueDisplayFormat { binary = 2, baseTen = 10, hex = 16, unicode = 99 };

#define GRID_SIZE 15

#define TOP_MARGIN 10
#define BOT_MARGIN TOP_MARGIN
#define SIDE_MARGIN TOP_MARGIN
#define RESIZE_CURSOR_MARGIN 10

#define IO_MIN_SPACING TOP_MARGIN / 2
#define IO_PIN_LEN SIDE_MARGIN / 2

#define BUTTON_INDENT 2

#define COMPONENT_COLUMN_MARGIN 40
#define COMPONENT_ROW_MARGIN COMPONENT_COLUMN_MARGIN / 2

#define WIRE_WIDTH 4
#define COMPONENT_BORDER_WIDTH 3

}  // namespace vsrtl
#endif  // VSRTL_GRAPHICS_DEFINES_H
