#ifndef VSRTL_Radix_H
#define VSRTL_Radix_H

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"
#include "vsrtl_port.h"

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QString>

namespace vsrtl {

enum class Radix { Hex, Unsigned, Signed, Binary, Enum };

static const auto hexRegex = QRegExp("0[xX][0-9a-fA-F]+");
static const auto binRegex = QRegExp("0[bB][0-1]+");
static const auto unsignedRegex = QRegExp("[0-9]+");
static const auto signedRegex = QRegExp("[-]*[0-9]+");

static inline VSRTL_VT_U decodePortRadixValue(const PortBase& port, const Radix type, const QString& valueString) {
    bool ok = false;
    VSRTL_VT_U value;
    switch (type) {
        case Radix::Hex: {
            value = valueString.toULong(&ok, 16);
            break;
        }
        case Radix::Binary: {
            QString trimmed = valueString;
            trimmed.remove(0, 2);  // Remove "0b" to allow decoding
            value = trimmed.toULong(&ok, 2);
            break;
        }
        case Radix::Unsigned: {
            value = valueString.toULong(&ok, 10);
            break;
        }
        case Radix::Signed: {
            // Zero extend the value, truncated at $port.getWidth()
            value = valueString.toLong(&ok, 10);
            // set zero as sign bit at $port.getWidth()
            value &= ~(0x1 << port.getWidth());
            // Sign extend from $port.getWidth()
            value = signextend(value, port.getWidth());
            break;
        }
        case Radix::Enum: {
            if (!port.isEnumPort()) {
                throw std::runtime_error("Port is not an Enum port");
            }
            value = port.enumStringToValue(valueString.toStdString().c_str());
            break;
        }
    }
    Q_ASSERT(ok);
    return value;
}

static inline QString encodePortRadixValue(const PortBase* port, const Radix type) {
    VSRTL_VT_U value = port->uValue();
    switch (type) {
        case Radix::Hex: {
            const unsigned maxChars = (port->getWidth() / 4) + (port->getWidth() % 4 != 0 ? 1 : 0);
            return "0x" + QString::number(value, 16).rightJustified(maxChars, '0');
        }
        case Radix::Binary: {
            return "0b" + QString::number(value, 2).rightJustified(port->getWidth(), '0');
        }
        case Radix::Unsigned: {
            return QString::number(value, 10);
        }
        case Radix::Signed: {
            return QString::number(signextend<VSRTL_VT_S>(value, port->getWidth()), 10);
        }
        case Radix::Enum: {
            if (!port->isEnumPort()) {
                throw std::runtime_error("Port is not an Enum port");
            }

            return QString::fromStdString(port->valueToEnumString());
        }
    }
}

static QMenu* createPortRadixMenu(const PortBase* port, Radix& type) {
    QMenu* menu = new QMenu("Radix");
    QActionGroup* RadixActionGroup = new QActionGroup(menu);

    QAction* enumTypeAction = nullptr;
    if (port->isEnumPort()) {
        enumTypeAction = RadixActionGroup->addAction("Enum");
        enumTypeAction->setCheckable(true);
        QObject::connect(enumTypeAction, &QAction::triggered, [&](bool checked) {
            if (checked)
                type = Radix::Enum;
        });
        menu->addAction(enumTypeAction);
    }

    QAction* hexTypeAction = RadixActionGroup->addAction("Hex");
    hexTypeAction->setCheckable(true);
    QObject::connect(hexTypeAction, &QAction::triggered, [&](bool checked) {
        if (checked)
            type = Radix::Hex;
    });
    menu->addAction(hexTypeAction);

    QAction* binTypeAction = RadixActionGroup->addAction("Binary");
    binTypeAction->setCheckable(true);
    QObject::connect(binTypeAction, &QAction::triggered, [&](bool checked) {
        if (checked)
            type = Radix::Binary;
    });
    menu->addAction(binTypeAction);

    QAction* unsignedTypeAction = RadixActionGroup->addAction("Unsigned");
    unsignedTypeAction->setCheckable(true);
    QObject::connect(unsignedTypeAction, &QAction::triggered, [&](bool checked) {
        if (checked)
            type = Radix::Unsigned;
    });
    menu->addAction(unsignedTypeAction);

    QAction* signedTypeAction = RadixActionGroup->addAction("Signed");
    signedTypeAction->setCheckable(true);
    QObject::connect(signedTypeAction, &QAction::triggered, [&](bool checked) {
        if (checked)
            type = Radix::Signed;
    });
    menu->addAction(signedTypeAction);

    RadixActionGroup->setExclusive(true);

    // Set the currently selected display type as checked
    switch (type) {
        case Radix::Hex: {
            hexTypeAction->setChecked(true);
            break;
        }
        case Radix::Binary: {
            binTypeAction->setChecked(true);
            break;
        }
        case Radix::Signed: {
            signedTypeAction->setChecked(true);
            break;
        }
        case Radix::Unsigned: {
            unsignedTypeAction->setChecked(true);
            break;
        }
        case Radix::Enum: {
            if (!port->isEnumPort()) {
                throw std::runtime_error("Port is not an Enum port");
            }
            enumTypeAction->setChecked(true);
            break;
        }
    }

    return menu;
}

}  // namespace vsrtl

#endif  // VSRTL_Radix_H
