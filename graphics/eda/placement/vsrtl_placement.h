#pragma once

#include "eda/vsrtl_tilegraph.h"

namespace vsrtl {
class GridComponent;

namespace eda {

class Placement {
public:
    /// Converts this placement into a tile graph.
    virtual std::shared_ptr<TileGraph> toTileGraph() const = 0;
};

// A placement with components assigned to physical positions.
class PhysicalPlacement : public Placement {
public:
    PhysicalPlacement(const std::vector<std::shared_ptr<ComponentTile>>& components = {});
    QRect componentBoundingRect() const;
    void addComponent(const std::shared_ptr<ComponentTile>& c) { m_components.push_back(c); }

    /// Based on this physical placement, creates a tile graph by the following algorithm:
    /// 1. Extrudes a set of horizontal and vertical lines from the chip rect and
    ///    component edges.
    /// 2. Based on the intersection of these lines, creates a set of tiles from the lines.
    /// 3. Builds a directed graph from these tiles.
    std::shared_ptr<TileGraph> toTileGraph() const override;

private:
    std::vector<std::shared_ptr<ComponentTile>> m_components;
};

// A placement with components ordered in a 2D matrix, without explicitly assigning physical positions to the
// components.
class MatrixPlacement : public Placement {
public:
    using GridComponentMatrix = std::map<int, std::vector<GridComponent*>>;
    MatrixPlacement(const GridComponentMatrix& matrix) : Placement(), matrix(matrix) {}
    unsigned width() const { return matrix.size(); }

    /// Returns the number of rows for 'column'.
    unsigned numRows(unsigned column) const;

    /// Returns the height of the matrix. If 'column' is set, assigns 'column' to the column containing
    /// boudning row height.
    unsigned height(unsigned* column = nullptr) const;
    std::shared_ptr<TileGraph> toTileGraph() const override;

private:
    GridComponentMatrix matrix;
};

// Placement algorithms
std::shared_ptr<Placement> MinCutPlacement(const std::vector<GridComponent*>& components);
std::shared_ptr<Placement> topologicalSortPlacement(const std::vector<GridComponent*>& components);
std::shared_ptr<Placement> ASAPPlacement(const std::vector<GridComponent*>& components);

}  // namespace eda
}  // namespace vsrtl
