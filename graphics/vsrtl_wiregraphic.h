#ifndef VSRTL_WIREGRAPHIC_H
#define VSRTL_WIREGRAPHIC_H

#include "vsrtl_component.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_portgraphic.h"

#include "cereal/cereal.hpp"
#include "cereal/include/cereal/types/map.hpp"
#include "cereal/include/cereal/types/string.hpp"
#include "cereal/include/cereal/types/utility.hpp"
#include "cereal/include/cereal/types/vector.hpp"

#include <QPen>
#include <memory>
#include <set>

namespace vsrtl {

class PortGraphic;
class PortBase;
class WireGraphic;
class WireSegment;
class ComponentGraphic;

static inline std::vector<std::string> getPortParentNameSeq(PortBase* p) {
    std::vector<std::string> seq;
    seq.push_back(p->getName());
    auto* parent = p->getParent();
    while (parent) {
        seq.push_back(parent->getName());
        parent = parent->getParent();
    }
    return seq;
}

/**
 * @brief The PointGraphic class
 * Base class for wire graphic points.
 */
class PointGraphic : public GraphicsBase {
public:
    PointGraphic(QGraphicsItem* parent);
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
};

/**
 * @brief The WirePoint class
 * A point on a wire. Shall display a context menu to delete. is select- and moveable.
 */
class WirePoint : public PointGraphic {
public:
    WirePoint(WireGraphic& parent, QGraphicsItem* sceneParent);

    void invalidate();
    void addOutputWire(WireSegment* wire) { m_outputWires.push_back(wire); }
    void clearOutputWires() { m_outputWires.clear(); }
    void setInputWire(WireSegment* wire) { m_inputWire = wire; }
    std::vector<WireSegment*>& getOutputWires() { return m_outputWires; }
    WireSegment* getInputWire() { return m_inputWire; }

    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void pointDrop(WirePoint* point);
    void pointDragEnter(WirePoint* point);
    void pointDragLeave(WirePoint* point);

private:
    WireGraphic& m_parent;
    ComponentGraphic* m_sceneParent;
    WireSegment* m_inputWire = nullptr;
    std::vector<WireSegment*> m_outputWires;
    WirePoint* m_draggedOnThis = nullptr;
};

class WireSegment : public GraphicsBase {
    friend class WirePoint;

public:
    WireSegment(WireGraphic* parent);

    void invalidate();
    void setStart(PointGraphic* start) { m_start = start; }
    void setEnd(PointGraphic* end) { m_end = end; }
    PointGraphic* getStart() const { return m_start; }
    PointGraphic* getEnd() const { return m_end; }
    QLineF getLine() const;

    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    QString deleted = "False";

private:
    PointGraphic* m_start = nullptr;
    PointGraphic* m_end = nullptr;
    WireGraphic* m_parent;
};

class WireGraphic : public GraphicsBase {
    friend class PortGraphic;

public:
    enum class MergeType { CannotMerge, MergeSinkWithSource, MergeSourceWithSink, MergeParallelSinks };

    WireGraphic(PortGraphic* from, const std::vector<PortBase*>& to, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    const QPen& getPen();
    void postSceneConstructionInitialize1() override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override {}

    PortGraphic* getFromPort() const { return m_fromPort; }
    const std::vector<PortGraphic*>& getToPorts() const { return m_toGraphicPorts; }
    std::pair<WirePoint*, WireSegment*> createWirePointOnSeg(const QPointF scenePos, WireSegment* onSegment);
    void removeWirePoint(WirePoint* point);

    bool managesPoint(WirePoint* point) const;
    void mergePoints(WirePoint* base, WirePoint* toMerge);
    MergeType canMergePoints(WirePoint* base, WirePoint* toMerge) const;

    void clearLayout();

    template <class Archive>
    void load(Archive& archive) {
        // Deserialize the layout
        std::pair<int, std::vector<std::string>> from;
        archive(cereal::make_nvp("From port", from));

        std::map<int, std::vector<std::string>> idxToOutportNameSeq;
        archive(cereal::make_nvp("To ports", idxToOutportNameSeq));

        std::map<int, QPoint> idxToPoints;
        archive(cereal::make_nvp("points", idxToPoints));

        std::vector<std::pair<int, int>> wires;
        archive(cereal::make_nvp("wires", wires));

        // Clear current layout
        for (const auto& p : m_points) {
            p->deleteLater();
        }
        m_points.clear();
        for (const auto& w : m_wires) {
            w->deleteLater();
        }
        m_wires.clear();

        std::map<int, PointGraphic*> idxToPort;

        // Locate input port
        if (from.second != getPortParentNameSeq(m_fromPort->getPort())) {
            throw std::runtime_error("Incompatible layout");
        }
        idxToPort[from.first] = m_fromPort->getPointGraphic();

        // Locate output ports
        for (const auto& iter : idxToOutportNameSeq) {
            Q_ASSERT(idxToPort.count(iter.first) == 0);
            PointGraphic* point = nullptr;
            for (const auto& p : m_toGraphicPorts) {
                if (getPortParentNameSeq(p->getPort()) == iter.second) {
                    point = p->getPointGraphic();
                    break;
                }
            }
            if (point == nullptr) {
                throw std::runtime_error("Incompatible layout");
            }
            idxToPort[iter.first] = point;
        }

        // Construct PointGraphic's
        for (const auto& p : idxToPoints) {
            Q_ASSERT(idxToPort.count(p.first) == 0);
            idxToPort[p.first] = createWirePoint();
        }

        // Construct wires
        for (const auto& w : wires) {
            if (idxToPort.count(w.first) == 0) {
                throw std::runtime_error("Wire start point not found");
            }
            if (idxToPort.count(w.second) == 0) {
                throw std::runtime_error("Wire end point not found");
            }
            auto* from = idxToPort[w.first];
            auto* to = idxToPort[w.second];

            auto* newSeg = createSegment(from, to);

            if (auto* g = dynamic_cast<WirePoint*>(from)) {
                g->addOutputWire(newSeg);
            }
            if (auto* g = dynamic_cast<WirePoint*>(to)) {
                g->setInputWire(newSeg);
            }
        }

        // Move wire points (must be done >after< the point has been associated with wires)
        for (const auto& p : idxToPort) {
            moveWirePoint(p.second, idxToPoints[p.first]);
        }
    }

    template <class Archive>
    void save(Archive& archive) const {
        Q_ASSERT(m_fromPort->getPortType() == PortType::out);

        int i = 0;
        // serialize the incoming port
        std::pair<int, std::vector<std::string>> from(i++, getPortParentNameSeq(m_fromPort->getPort()));
        archive(cereal::make_nvp("From port", from));

        // serialize the outgoing, connecting ports
        std::map<int, std::vector<std::string>> idxToOutportNameSeq;
        std::map<PointGraphic*, int> outportToIdx;
        for (const auto& p : m_toGraphicPorts) {
            // @todo: this is not sufficient, ports may be named identically.
            // It should be a hierarchical list including its parent components
            idxToOutportNameSeq[i] = getPortParentNameSeq(p->getPort());
            outportToIdx[p->getPointGraphic()] = i;
            i++;
        }
        archive(cereal::make_nvp("To ports", idxToOutportNameSeq));

        // Each point managed by this wire is enumerated and associated with its position
        std::map<int, QPoint> idxToPos;
        std::map<PointGraphic*, int> pointToIdx;
        for (const auto& p : m_points) {
            idxToPos[i] = p->pos().toPoint();
            pointToIdx[p] = i;
            i++;
        }
        archive(cereal::make_nvp("points", idxToPos));

        // Each wire segment will connect to either a point or a port
        std::vector<std::pair<int, int>> wires;
        for (auto& w : m_wires) {
            auto* start = w->getStart();
            auto* end = w->getEnd();

            int startIdx, endIdx;

            if (m_fromPort->getPointGraphic() == start) {
                startIdx = 0;
            } else if (outportToIdx.count(start)) {
                startIdx = outportToIdx[start];
            } else if (pointToIdx.count(start)) {
                startIdx = pointToIdx[start];
            } else {
                Q_ASSERT(false && "Could not serialize wire");
            }

            if (outportToIdx.count(end)) {
                endIdx = outportToIdx[end];
            } else if (pointToIdx.count(end)) {
                endIdx = pointToIdx[end];
            } else {
                Q_ASSERT(false && "Could not serialize wire");
            }
            wires.push_back({startIdx, endIdx});
        }
        archive(cereal::make_nvp("wires", wires));
    }

private:
    ComponentGraphic* getPointOwningComponent();
    WirePoint* createWirePoint();
    void moveWirePoint(PointGraphic* point, const QPointF scenePos);
    WireSegment* createSegment(PointGraphic* start, PointGraphic* end);
    void createRectilinearSegments(PointGraphic* start, PointGraphic* end);

    PortGraphic* m_fromPort = nullptr;
    std::vector<PortBase*> m_toPorts;
    std::vector<PortGraphic*> m_toGraphicPorts;
    std::set<WireSegment*> m_wires;
    std::set<WirePoint*> m_points;
};
}  // namespace vsrtl

#endif  // VSRTL_WIREGRAPHIC_H
