#ifndef VSRTL_Radix_H
#define VSRTL_Radix_H

#include "../interface/vsrtl_defines.h"
#include <QRegularExpression>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace vsrtl {
class SimPort;
enum class Radix { Hex, Unsigned, Signed, Binary, Enum };

static const auto hexRegex = QRegularExpression("0[xX][0-9a-fA-F]+");
static const auto binRegex = QRegularExpression("0[bB][0-1]+");
static const auto unsignedRegex = QRegularExpression("[0-9]+");
static const auto signedRegex = QRegularExpression("[-]*[0-9]+");

VSRTL_VT_U decodePortRadixValue(const SimPort &port, const Radix type,
                                const QString &valueString);
QString encodePortRadixValue(const SimPort *port, const Radix type);
QMenu *createPortRadixMenu(const SimPort *port, Radix &type);

} // namespace vsrtl

#endif // VSRTL_Radix_H
