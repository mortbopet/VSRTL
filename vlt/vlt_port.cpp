#include "vlt_port.h"

namespace vsrtl {
namespace vlt {

Port::Port(const std::string& name, unsigned width, SimComponent* parent) : SimPort(name, parent), m_width(width) {}

}  // namespace vlt
}  // namespace vsrtl
