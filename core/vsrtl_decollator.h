#ifndef VSRTL_DECOLLATOR_H
#define VSRTL_DECOLLATOR_H

#include "vsrtl_component.h"

namespace vsrtl {


/**
 * @brief The Decollator class
 *           _____
 *           |   | s[0]
 * [3:0] s-> |   | s[1]
 *           |   | s[2]
 *           |___| d[3]
 */
class Decollator : public Component {
public:
    Decollator(std::string name, unsigned int width) : Component(name), m_width(width) {
        out = this->createOutputPorts("out", width, 1);
        in.setWidth(width);

        for(int i = 0; i < width; i++){
            *out[i] << [=] {
                return (static_cast<VSRTL_VT_U>(in) >> i) & 0b1;
            };
        }

    }
    OUTPUTPORTS(out);
    INPUTPORT(in);

protected:
    unsigned int m_width;
};

}

#endif // VSRTL_DECOLLATOR_H
