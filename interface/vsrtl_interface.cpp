#include "vsrtl_interface.h"

namespace vsrtl {
void SimPort::queueVcdVarChange() { getDesign()->queueVcdVarChange(this); }

SimDesign *SimBase::getDesign() {
  if (m_design)
    return m_design;

  // m_design has yet to be initialized. (This cannot be done during
  // construction, so we lazily initialize the design pointer upon the first
  // request to such). Recurse until locating a parent which either has its
  // SimDesign set, or the component has no parent (ie. it is the design)
  if (!m_parent) {
    m_design = dynamic_cast<SimDesign *>(this);
  } else {
    m_design = m_parent->getDesign();
  }
  assert(m_design != nullptr);

  return m_design;
}
} // namespace vsrtl
