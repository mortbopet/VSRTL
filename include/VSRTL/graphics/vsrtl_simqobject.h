#pragma once

#include <QObject>

#include <memory>
#include <vector>

#include "gallantsignalwrapper.h"

/// The SimQObject class acts as a base class for graphics components which need
/// the gallant-to-Qt signal translation mechanism, for using simulator signals
/// in a Qt signal/slot like way.

namespace vsrtl {

class SimQObject : public QObject {
public:
  /// This function translated a simulator signal object into Qt signals.
  /// simUpdateSlot will be called whenever the simulator signal is emitted.
  template <typename D>
  void wrapSimSignal(D &sig, Qt::ConnectionType type = Qt::AutoConnection) {
    m_simSignalWrappers.push_back(
        std::unique_ptr<GallantSignalWrapperBase>(new GallantSignalWrapper(
            this, [this]() { this->simUpdateSlot(); }, sig, type)));
  }

  /// Connect wrappers for making model signal emissions threadsafe Qt signals.
  std::vector<std::unique_ptr<GallantSignalWrapperBase>> m_simSignalWrappers;

  /// The slot called whenever the associated simulator object announced that
  /// its state has changed.
  virtual void simUpdateSlot() {}
};

} // namespace vsrtl
