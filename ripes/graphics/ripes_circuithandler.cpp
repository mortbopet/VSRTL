#include "ripes_circuithandler.h"
#include "ripes_graphics_defines.h"

#include <QDebug>
namespace ripes {

// A reverse-direction map - index [column][row]
typedef std::map<int, std::map<int, Component*>> Matrix;

CircuitHandler::CircuitHandler(RipesView* view) : m_view(view) {}

bool getPos(const Matrix& m, const Component* c_ptr, std::pair<int, int>& pair) {
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

void setMatrixPos(Matrix& m, Component* ptr, const std::pair<int, int> pos) {
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

void CircuitHandler::orderSubcomponents(const ComponentGraphic* parent) {
    Matrix matrix;
    std::set<Component*> processedComponents;
    const auto subComponents = parent->getComponent()->getSubComponents();

    // Start matrix ordering the component graph with a component that has an input
    auto c_iter = subComponents.begin();
    while (c_iter != subComponents.end() && (*c_iter++)->getInputComponents().size() == 0) {
    }
    if (c_iter == subComponents.end()) {
        assert(false);
    }
    const std::pair<int, int> initialPos(0, 0);
    setMatrixPos(matrix, *c_iter, initialPos);

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

    // Position components
    int xPos = 0;
    for (const auto& c : matrix) {
        int yPos = 0;
        for (const auto& r : c.second) {
            ComponentGraphic* g = m_view->lookupGraphicForComponent(matrix[c.first][r.first]);
            if (g) {
                if (r.first > 0) {
                    yPos += m_view->lookupGraphicForComponent(matrix[c.first][r.first - 1])->boundingRect().height() +
                            COMPONENT_ROW_MARGIN;
                }
                // Center component in column
                g->setPos(xPos + (columnWidths[c.first] - g->boundingRect().width()) / 2, yPos);
            }
        }
        xPos += columnWidths[c.first] + COMPONENT_COLUMN_MARGIN;
    }
}
}
