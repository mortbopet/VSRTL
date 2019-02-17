#ifndef VSRTL_GRAPHICS_DEFINES_H
#define VSRTL_GRAPHICS_DEFINES_H

namespace vsrtl {

enum class ValueDisplayFormat { binary = 2, baseTen = 10, hex = 16, unicode = 99 };

#define GRID_SIZE 15

#define TOP_MARGIN 10
#define BOT_MARGIN TOP_MARGIN
#define SIDE_MARGIN TOP_MARGIN
#define RESIZE_CURSOR_MARGIN 10

#define IO_MIN_SPACING TOP_MARGIN / 2
#define IO_PIN_LEN SIDE_MARGIN / 2

#define SHADOW_OFFSET SIDE_MARGIN / 8
#define SHADOW_WIDTH 1

#define BUTTON_INDENT 2

#define COMPONENT_COLUMN_MARGIN 40
#define COMPONENT_ROW_MARGIN COMPONENT_COLUMN_MARGIN / 2
}  // namespace vsrtl
#endif  // VSRTL_GRAPHICS_DEFINES_H
