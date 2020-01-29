#include "vsrtl_widget.h"
#include "../interface/vsrtl_gfxobjecttypes.h"
#include "ui_vsrtl_widget.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_scene.h"
#include "vsrtl_shape.h"
#include "vsrtl_view.h"

#include <memory>

#include <QGraphicsScene>

namespace vsrtl {

VSRTLWidget::VSRTLWidget(QWidget* parent) : QWidget(parent), ui(new Ui::VSRTLWidget) {
    ui->setupUi(this);

    m_view = new VSRTLView(this);
    m_view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    m_scene = new VSRTLScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, (&VSRTLWidget::handleSceneSelectionChanged));
}

void VSRTLWidget::clearDesign() {
    if (m_topLevelComponent) {
        // Clear previous design
        m_topLevelComponent->deleteLater();
    }
    m_design = nullptr;
}

void VSRTLWidget::setDesign(SimDesign* design) {
    if (m_topLevelComponent) {
        // Clear previous design
        m_topLevelComponent->deleteLater();
    }
    m_design = design;
    initializeDesign();

    setLocked(m_scene->isLocked());
}

void VSRTLWidget::setOutputPortValuesVisible(bool visible) {
    m_scene->setPortValuesVisibleForType(PortType::out, visible);
}

void VSRTLWidget::setLocked(bool locked) {
    m_scene->setLocked(locked);
}

VSRTLWidget::~VSRTLWidget() {
    delete ui;
}

void VSRTLWidget::setShowPortWidth(bool visible) {
    m_scene->setShowPortWidth(visible);
}

void VSRTLWidget::handleSceneSelectionChanged() {
    std::vector<SimComponent*> selectedComponents;
    std::vector<SimPort*> selectedPorts;
    for (const auto& i : m_scene->selectedItems()) {
        ComponentGraphic* i_c = dynamic_cast<ComponentGraphic*>(i);
        if (i_c) {
            selectedComponents.push_back(&i_c->getComponent());
            continue;
        }
        PortGraphic* i_p = dynamic_cast<PortGraphic*>(i);
        if (i_p) {
            selectedPorts = i_p->getPort()->getPortsInConnection();
            continue;
        }
    }

    emit componentSelectionChanged(selectedComponents);
    emit portSelectionChanged(selectedPorts);
}  // namespace vsrtl

void VSRTLWidget::handleSelectionChanged(const std::vector<SimComponent*>& selected,
                                         std::vector<SimComponent*>& deselected) {
    // Block signals from scene to disable selectionChange emission.
    m_scene->blockSignals(true);
    for (const auto& c : selected) {
        auto* c_g = c->getGraphic<ComponentGraphic>();
        c_g->setSelected(true);
    }
    for (const auto& c : deselected) {
        auto* c_g = c->getGraphic<ComponentGraphic>();
        c_g->setSelected(false);
    }
    m_scene->blockSignals(false);
}

void VSRTLWidget::initializeDesign() {
    // Verify the design in case user forgot to
    m_design->verifyAndInitialize();

    // Create a ComponentGraphic for the top component. This will expand the component tree and create graphics for all
    // ports, wires etc. within the design. This is done through the initialize call, which must be called after the
    // item has been added to the scene.
    m_topLevelComponent = new ComponentGraphic(*m_design, nullptr);
    addComponent(m_topLevelComponent);
    m_topLevelComponent->initialize();
    // At this point, all graphic items have been created, and the post scene construction initialization may take
    // place. Similar to the initialize call, postSceneConstructionInitialization will recurse through the entire tree
    // which is the graphics items in the scene.
    m_topLevelComponent->postSceneConstructionInitialize1();
    m_topLevelComponent->postSceneConstructionInitialize2();

    // Expand top widget
    m_topLevelComponent->setExpanded(true);
}

void VSRTLWidget::expandAllComponents(ComponentGraphic* fromThis) {
    if (fromThis == nullptr)
        fromThis = m_topLevelComponent;

    // Components are expanded and routed from leaf nodes and up
    for (const auto& sub : fromThis->getGraphicSubcomponents())
        expandAllComponents(sub);

    fromThis->setExpanded(true);
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

void VSRTLWidget::run() {
    if (m_design) {
        m_design->setEnableSignals(false);
        while (!m_stop) {
            m_design->clock();
        }
        m_stop = false;
        m_design->setEnableSignals(true);
        m_scene->update();
    }
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

void VSRTLWidget::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}  // namespace vsrtl
