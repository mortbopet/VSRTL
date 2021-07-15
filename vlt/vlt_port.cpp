#include "vlt_port.h"

namespace vsrtl {
namespace vlt {

Port::Port(const std::string& name, unsigned width, PortType type, SimComponent* parent)
    : SimPort(name, parent, type), m_width(width) {}

}  // namespace vlt
}  // namespace vsrtl
