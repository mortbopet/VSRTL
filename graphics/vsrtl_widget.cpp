#include "vsrtl_widget.h"
#include "../interface/vsrtl_gfxobjecttypes.h"
#include "ui_vsrtl_widget.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_scene.h"
#include "vsrtl_shape.h"
#include "vsrtl_view.h"

#include <memory>

#include <QFontDatabase>
#include <QGraphicsScene>
#include <QtConcurrent/QtConcurrent>

void initVsrtlResources() {
  Q_INIT_RESOURCE(vsrtl_icons);
  Q_INIT_RESOURCE(vsrtl_fonts);

  // Initialize fonts
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Thin.ttf");
  QFontDatabase::addApplicationFont(
      ":/vsrtl_fonts/Roboto/Roboto-ThinItalic.ttf");
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont(
      ":/vsrtl_fonts/Roboto/Roboto-MediumItalic.ttf");
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Medium.ttf");
  QFontDatabase::addApplicationFont(
      ":/vsrtl_fonts/Roboto/Roboto-LightItalic.ttf");
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Light.ttf");
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Italic.ttf");
  QFontDatabase::addApplicationFont(
      ":/vsrtl_fonts/Roboto/Roboto-BoldItalic.ttf");
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Bold.ttf");
  QFontDatabase::addApplicationFont(":/vsrtl_fonts/Roboto/Roboto-Black.ttf");
  QFontDatabase::addApplicationFont(
      ":/vsrtl_fonts/Roboto/Roboto-BlackItalic.ttf");
}

namespace vsrtl {

VSRTLWidget::VSRTLWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::VSRTLWidget) {
  ui->setupUi(this);
  initVsrtlResources();

  m_view = new VSRTLView(this);
  m_view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
  m_scene = new VSRTLScene(this);
  m_view->setScene(m_scene);
  ui->viewLayout->addWidget(m_view);
  connect(m_scene, &QGraphicsScene::selectionChanged, this,
          (&VSRTLWidget::handleSceneSelectionChanged));

  /**
   * runFinished will always be emitted asynchronously within the run call.
   * When a run is finished, we need to ensure that the graphical view is fully
   * up to date with the underlying circuit. This is performed through sync, but
   * ensured to be performed on the main gui thread, and not on whatever thread
   * the running was performed on.
   */
  connect(this, &VSRTLWidget::runFinished, this, &VSRTLWidget::sync,
          Qt::QueuedConnection);
}

void VSRTLWidget::clearDesign() {
  if (m_topLevelComponent) {
    // Clear previous design
    delete m_topLevelComponent;
    m_topLevelComponent = nullptr;
  }
  m_design = nullptr;
}

void VSRTLWidget::setDesign(SimDesign *design, bool doPlaceAndRoute) {
  clearDesign();
  m_design = design;
  initializeDesign(doPlaceAndRoute);
  setLocked(m_scene->isLocked());
  setDarkmode(m_scene->darkmode());
}

void VSRTLWidget::zoomToFit() { m_view->zoomToFit(getTopLevelComponent()); }

void VSRTLWidget::setOutputPortValuesVisible(bool visible) {
  m_scene->setPortValuesVisibleForType(vsrtl::SimPort::PortType::out, visible);
}

void VSRTLWidget::setDarkmode(bool enabled) { m_scene->setDarkmode(enabled); }

void VSRTLWidget::setLocked(bool locked) { m_scene->setLocked(locked); }

VSRTLWidget::~VSRTLWidget() { delete ui; }

void VSRTLWidget::handleSceneSelectionChanged() {
  std::vector<SimComponent *> selectedComponents;
  std::vector<SimPort *> selectedPorts;
  const auto selectedItems = m_scene->selectedItems();
  for (auto *i : std::as_const(selectedItems)) {
    ComponentGraphic *i_c = dynamic_cast<ComponentGraphic *>(i);
    if (i_c) {
      selectedComponents.push_back(i_c->getComponent());
      continue;
    }
    PortGraphic *i_p = dynamic_cast<PortGraphic *>(i);
    if (i_p) {
      selectedPorts = i_p->getPort()->getPortsInConnection();
      continue;
    }
  }

  emit componentSelectionChanged(selectedComponents);
  emit portSelectionChanged(selectedPorts);
} // namespace vsrtl

void VSRTLWidget::handleSelectionChanged(
    const std::vector<SimComponent *> &selected,
    const std::vector<SimComponent *> &deselected) {
  // Block signals from scene to disable selectionChange emission.
  m_scene->blockSignals(true);
  for (const auto &c : selected) {
    auto *c_g = c->getGraphic<ComponentGraphic>();
    c_g->setSelected(true);
  }
  for (const auto &c : deselected) {
    auto *c_g = c->getGraphic<ComponentGraphic>();
    c_g->setSelected(false);
  }
  m_scene->blockSignals(false);
}

void VSRTLWidget::initializeDesign(bool doPlaceAndRoute) {
  // Verify the design in case user forgot to
  m_design->verifyAndInitialize();

  // Create a ComponentGraphic for the top component. This will expand the
  // component tree and create graphics for all ports, wires etc. within the
  // design. This is done through the initialize call, which must be called
  // after the item has been added to the scene.
  m_topLevelComponent = new ComponentGraphic(m_design, nullptr);
  m_topLevelComponent->initialize(doPlaceAndRoute);
  // At this point, all graphic items have been created, and the post scene
  // construction initialization may take place. Similar to the initialize call,
  // postSceneConstructionInitialization will recurse through the entire tree
  // which is the graphics items in the scene.
  m_topLevelComponent->postSceneConstructionInitialize1();
  m_topLevelComponent->postSceneConstructionInitialize2();

  // Expand top widget
  m_topLevelComponent->setExpanded(true);

  // Add top level component to scene at the end. Do _not_ move this before
  // initialization - initialization will be massively slowed down if items are
  // modified while already in the scene.
  addComponent(m_topLevelComponent);
}

void VSRTLWidget::expandAllComponents(ComponentGraphic *fromThis) {
  if (fromThis == nullptr)
    fromThis = m_topLevelComponent;

  fromThis->setExpanded(true);

  // Components are expanded and routed from leaf nodes and up
  for (const auto &sub : fromThis->getGraphicSubcomponents())
    expandAllComponents(sub);

  fromThis->placeAndRouteSubcomponents();
}

bool VSRTLWidget::isReversible() {
  if (!m_design)
    return false;

  if (m_designCanreverse != m_design->canReverse()) {
    // Reverse state just changed, notify listeners
    m_designCanreverse = m_design->canReverse();
    emit canReverse(m_designCanreverse);
  }
  return m_designCanreverse;
}

void VSRTLWidget::clock() {
  if (m_design) {
    m_design->clock();
    isReversible();
  }
}

void VSRTLWidget::sync() {
  // Since the design does not emit signals during running, we need to manually
  // tell all labels to reset their text value, given that labels manually must
  // have their text updated (ie. text is not updated in the redraw call).
  const auto sceneItems = m_scene->items();
  for (auto *item : std::as_const(sceneItems)) {
    if (auto *simobject = dynamic_cast<SimQObject *>(item)) {
      simobject->simUpdateSlot();
    }
  }

  m_scene->update();
}

QFuture<void> VSRTLWidget::run(const std::function<void()> &cycleFunctor) {
  auto future = QtConcurrent::run([=] {
    if (m_design) {
      m_design->setEnableSignals(false);
      if (cycleFunctor) {
        while (!m_stop) {
          m_design->clock();
          cycleFunctor();
        }
      } else {
        while (!m_stop) {
          m_design->clock();
        }
      }
      m_stop = false;
      m_design->setEnableSignals(true);

      emit runFinished();
    }
  });
  return future;
}

void VSRTLWidget::reverse() {
  if (m_design) {
    m_design->reverse();
    isReversible();
  }
}

void VSRTLWidget::reset() {
  if (m_design) {
    m_design->reset();
    isReversible();
  }
}

void VSRTLWidget::addComponent(ComponentGraphic *g) { m_scene->addItem(g); }
} // namespace vsrtl
