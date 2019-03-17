#ifndef VSRTL_DISPLAYTYPE_H
#define VSRTL_DISPLAYTYPE_H

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"

#include <QString>

namespace vsrtl {

enum class DisplayType { Hex, Unsigned, Signed, Binary };

static const auto hexRegex = QRegExp("0[xX][0-9a-fA-F]+");
static const auto binRegex = QRegExp("0[bB][0-1]+");
static const auto unsignedRegex = QRegExp("[0-9]+");
static const auto signedRegex = QRegExp("[-]*[0-9]+");

static inline VSRTL_VT_U decodeDisplayValue(const QString& valueString, int width, DisplayType t) {
    bool ok = false;
    VSRTL_VT_U value;
    switch (t) {
        case DisplayType::Hex: {
            value = valueString.toULong(&ok, 16);
            break;
        }
        case DisplayType::Binary: {
            QString trimmed = valueString;
            trimmed.remove(0, 2);  // Remove "0b" to allow decoding
            value = trimmed.toULong(&ok, 2);
            break;
        }
        case DisplayType::Unsigned: {
            value = valueString.toULong(&ok, 10);
            break;
        }
        case DisplayType::Signed: {
            // Zero extend the value, truncated at $width
            value = valueString.toLong(&ok, 10);
            // set zero as sign bit at $width
            value &= ~(0x1 << width);
            // Sign extend from $width
            value = signextend(value, width);
            break;
        }
    }
    Q_ASSERT(ok);
    return value;
}

static inline QString encodeDisplayValue(VSRTL_VT_U value, int width, DisplayType t) {
    switch (t) {
        case DisplayType::Hex: {
            return "0x" + QString::number(value, 16).rightJustified(8, '0');
        }
        case DisplayType::Binary: {
            return "0b" + QString::number(value, 2).rightJustified(8, '0');
        }
        case DisplayType::Unsigned: {
            return QString::number(value, 10);
        }
        case DisplayType::Signed: {
            return QString::number(signextend<VSRTL_VT_S>(value, width), 10);
        }
    }
}

}  // namespace vsrtl

#endif  // VSRTL_DISPLAYTYPE_H
