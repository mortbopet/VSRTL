#ifndef VSRTL_DECOLLATOR_H
#define VSRTL_DECOLLATOR_H

#include "vsrtl_component.h"
#include "vsrtl_port.h"

namespace vsrtl {

/**
 * @brief The Decollator class
 *           _____
 *           |   | s[0]
 * [3:0] s-> |   | s[1]
 *           |   | s[2]
 *           |___| d[3]
 */
template <unsigned int W>
class Decollator : public Component {
public:
    Decollator(std::string name, Component* parent) : Component(name, parent) {
        for (int i = 0; i < W; i++) {
            *out[i] << [=] { return (static_cast<VSRTL_VT_U>(in) >> i) & 0b1; };
        }
    }

    OUTPUTPORTS(out, 1, W);
    INPUTPORT(in, W);
};

}  // namespace vsrtl

#endif  // VSRTL_DECOLLATOR_H
