#include "vsrtl_circuithandler.h"
#include "vsrtl_graphics_defines.h"

#include <deque>

#include <QDebug>
namespace vsrtl {

// A reverse-direction map - index [column][row]
typedef std::map<int, std::map<int, Component*>> ComponentMatrix;

CircuitHandler::CircuitHandler(VSRTLView* view) : m_view(view) {}

bool getPos(const ComponentMatrix& m, const Component* c_ptr, std::pair<int, int>& pair) {
    for (const auto& c : m) {
        for (const auto& r : c.second) {
            if (r.second == c_ptr) {
                pair = std::pair<int, int>(c.first, r.first);
                return true;
            }
        }
    }
    return false;
}

void setMatrixPos(ComponentMatrix& m, Component* ptr, const std::pair<int, int> pos) {
    m[pos.first][pos.second] = ptr;

    auto inputComponents = ptr->getInputComponents();
    std::pair<int, int> newPos;
    for (const auto& c : inputComponents) {
        if (!getPos(m, c, newPos)) {
            // Add component to matrix
            newPos = std::pair<int, int>(pos.first - 1, m[pos.first - 1].size());
            m[newPos.first][newPos.second] = c;
            setMatrixPos(m, c, newPos);
        }
    }
}

/**
 * @brief CircuitHandler::orderSubcomponents
 * Subcomponent ordering is based on a topological sort algorithm.
 * This algorithm is applicable for DAG's (Directed Acyclical Graph's). Naturally, digital circuits does not fulfull the
 * requirement for being a DAG. However, if Registers are seen as having either their input or output disregarded as an
 * edge in a DAG, a DAG can be created from a digital circuit, if only outputs are considered.
 * Having a topological sorting of the components, rows- and columns can
 * be generated for each depth in the topological sort, wherein the components are finally layed out.
 * @param parent
 */

namespace {
void orderSubcomponentsUtil(Component* c, std::map<Component*, bool>& visited, std::deque<Component*>& stack) {
    visited[c] = true;

    for (const auto& cc : c->getOutputComponents()) {
        if (!visited[cc]) {
            orderSubcomponentsUtil(cc, visited, stack);
        }
    }
    stack.push_front(c);
}

}  // namespace

void CircuitHandler::orderSubcomponents(const ComponentGraphic* parent) {
    const auto& subComponents = parent->getComponent().getSubComponents();
    std::map<Component*, bool> visited;
    std::deque<Component*> stack;

    for (const auto& cpt : subComponents)
        visited[cpt.get()] = false;

    for (const auto& c : visited) {
        if (!c.second) {
            orderSubcomponentsUtil(c.first, visited, stack);
        }
    }

    for (const auto& c : stack)
        std::cout << c->getName() << std::endl;

    /*
    std::set<Component*> processedComponents;

    ComponentMatrix matrix;
    // Start matrix ordering the component graph with a component that has an input
    auto c_iter = subComponents.begin();
    while (c_iter != subComponents.end() && (*c_iter++)->getInputComponents().size() == 0) {
    }
    if (c_iter == subComponents.end()) {
        assert(false);
    }
    const std::pair<int, int> initialPos(0, 0);
    setMatrixPos(matrix, c_iter->get(), initialPos);

    // Calculate row widths
    std::map<int, int> columnWidths;
    for (const auto& c : matrix) {
        int width = -1;
        for (const auto& r : c.second) {
            ComponentGraphic* g = m_view->lookupGraphicForComponent(matrix[c.first][r.first]);
            if (g) {
                width = width > g->boundingRect().width() ? width : g->boundingRect().width();
            }
        }
        columnWidths[c.first] = width;
    }

    // calculate middelposition
    */

    // Position components
    int xPos = 0;
    for (const auto& c : stack) {
        int yPos = 0;
        ComponentGraphic* g = m_view->lookupGraphicForComponent(c);
        if (g) {
            // Center component in column
            // g->setPos(xPos + (columnWidths[c.first] - g->boundingRect().width()) / 2, yPos);
            xPos += g->boundingRect().width();
            g->setPos(xPos, yPos);
        }
        // xPos += columnWidths[c.first] + COMPONENT_COLUMN_MARGIN;
    }
}
}  // namespace vsrtl
