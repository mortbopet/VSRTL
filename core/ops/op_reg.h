#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"
#include "vsrtl_register.h"

namespace vsrtl {
namespace core {

class OpReg : public RegisterBase {
public:
    SetGraphicsType(Register);

    OpReg(std::string name, SimComponent* parent, unsigned int W) : RegisterBase(name, parent), m_W(W) {
        DYNP_IN_INIT(in, m_W);
        DYNP_OUT_INIT(out, m_W);

        setSpecialPort("in", getIn());
        setSpecialPort("out", getOut());

        // Calling out.propagate() will clock the register the register
        *out << ([=] { return m_savedValue; });
    }

    void setInitValue(VSRTL_VT_U value) { m_initvalue = value; }

    void reset() override {
        m_savedValue = m_initvalue;
        m_reverseStack.clear();
    }

    void save() override {
        saveToStack();
        m_savedValue = in->value<VSRTL_VT_U>();
    }

    void forceValue(VSRTL_VT_U /* addr */, VSRTL_VT_U value) override {
        // Sign-extension with unsigned type forces width truncation to m_width bits
        m_savedValue = signextend<VSRTL_VT_U>(value, m_W);
        // Forced values are a modification of the current state and thus not pushed onto the reverse stack
    }

    void reverse() override {
        if (m_reverseStack.size() > 0) {
            m_savedValue = m_reverseStack.front();
            m_reverseStack.pop_front();
        }
    }

    Port* getIn() override { return in; }
    Port* getOut() override { return out; }

    DYNP_IN(in);
    DYNP_OUT(out);

    void reverseStackSizeChanged() override {
        if (reverseStackSize() < m_reverseStack.size()) {
            m_reverseStack.resize(m_reverseStack.size());
        }
    }

protected:
    void saveToStack() {
        m_reverseStack.push_front(m_savedValue);
        if (m_reverseStack.size() > reverseStackSize()) {
            m_reverseStack.pop_back();
        }
    }

    unsigned int m_W;
    VSRTL_VT_U m_savedValue = 0;
    VSRTL_VT_U m_initvalue = 0;
    std::deque<VSRTL_VT_U> m_reverseStack;
};

}  // namespace core
}  // namespace vsrtl
