#ifndef VSRTL_WIREGRAPHIC_H
#define VSRTL_WIREGRAPHIC_H

#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_portgraphic.h"

#include "cereal/cereal.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/utility.hpp"
#include "cereal/types/vector.hpp"

#include <QPen>
#include <memory>
#include <set>

namespace vsrtl {

class PortGraphic;
class WireGraphic;
class WireSegment;
class ComponentGraphic;

static inline std::vector<std::string> getPortParentNameSeq(SimPort* p) {
    std::vector<std::string> seq;
    seq.push_back(p->getName());
    seq.push_back(p->getParent()->getName());
    return seq;
}

/**
 * @brief The PortPoint class
 * Base class for wire graphic points. This is the point type which is assigned to PortGraphics. They are not moveable,
 * but provide an interface between moveable WirePoints (on WireSegments) and immovable PortPoints, on ports.
 */
class PortPoint : public GraphicsBase {
public:
    PortPoint(QGraphicsItem* parent);
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    // Must be returned by copy; may get used to invalidate a wire, wherein each output wire will get dereferenced with
    // this point (and hence modifying m_outputWires)
    std::vector<WireSegment*> getOutputWires() { return m_outputWires; }
    WireSegment* getInputWire() { return m_inputWire; }

    void addOutputWire(WireSegment* wire) { m_outputWires.push_back(wire); }
    void removeOutputWire(WireSegment* wire);
    void clearOutputWires() { m_outputWires.clear(); }
    void setInputWire(WireSegment* wire) { m_inputWire = wire; }
    void clearInputWire() { m_inputWire = nullptr; }

protected:
    WireSegment* m_inputWire = nullptr;
    std::vector<WireSegment*> m_outputWires;
};

/**
 * @brief The WirePoint class
 * A point on a wire. Shall display a context menu to delete. is select- and moveable.
 */
class WirePoint : public PortPoint {
public:
    WirePoint(WireGraphic& parent, QGraphicsItem* sceneParent);

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
    WirePoint* m_draggedOnThis = nullptr;
};

class WireSegment : public GraphicsBase {
    friend class WirePoint;

public:
    WireSegment(WireGraphic* parent);

    void invalidate();
    void setStart(PortPoint* start);
    void setEnd(PortPoint* end);
    PortPoint* getStart() const { return m_start; }
    PortPoint* getEnd() const { return m_end; }
    QLineF getLine() const;

    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    QString deleted = "False";

private:
    PortPoint* m_start = nullptr;
    PortPoint* m_end = nullptr;
    WireGraphic* m_parent;
};

class WireGraphic : public GraphicsBase {
    friend class PortGraphic;

public:
    enum class MergeType { CannotMerge, MergeSinkWithSource, MergeSourceWithSink, MergeParallelSinks };

    WireGraphic(PortGraphic* from, const std::vector<SimPort*>& to, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    const QPen& getPen();
    void postSceneConstructionInitialize1() override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override {}
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void setWiresVisibleToPort(const PortPoint* p, bool visible);
    void portMoved(const PortGraphic* port, const QPoint dP);
    PortGraphic* getFromPort() const { return m_fromPort; }
    const std::vector<PortGraphic*>& getToPorts() const { return m_toGraphicPorts; }
    std::pair<WirePoint*, WireSegment*> createWirePointOnSeg(const QPointF scenePos, WireSegment* onSegment);
    void removeWirePoint(WirePoint* point);
    void clearWirePoints();

    bool managesPoint(WirePoint* point) const;
    void mergePoints(WirePoint* base, WirePoint* toMerge);
    MergeType canMergePoints(WirePoint* base, WirePoint* toMerge) const;

    void clearLayout();

    template <class Archive>
    void load(Archive& archive) {
        // Deserialize the layout

        // This layout might originate from a similar component, but with a different name. Get the serialized parent
        // name as a reference for what must be exchanged with the current parent name
        std::string inParent;
        archive(cereal::make_nvp("Parent", inParent));

        std::pair<int, std::vector<std::string>> from;
        archive(cereal::make_nvp("From port", from));
        std::replace(from.second.begin(), from.second.end(), inParent, getPointOwningComponent().getName());

        std::map<int, std::vector<std::string>> idxToOutportNameSeq;
        archive(cereal::make_nvp("To ports", idxToOutportNameSeq));
        for (auto& iter : idxToOutportNameSeq) {
            std::replace(iter.second.begin(), iter.second.end(), inParent, getPointOwningComponent().getName());
        }

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

        std::map<int, PortPoint*> idxToPort;

        // Locate input port
        const auto fromPortSeq = getPortParentNameSeq(m_fromPort->getPort());
        if (from.second != fromPortSeq) {
            throw std::runtime_error("Incompatible layout");
        }
        idxToPort[from.first] = m_fromPort->getPointGraphic();

        // Locate output ports
        for (const auto& iter : idxToOutportNameSeq) {
            Q_ASSERT(idxToPort.count(iter.first) == 0);
            PortPoint* point = nullptr;
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
            p.second->setPos(idxToPoints[p.first]);
        }
    }

    template <class Archive>
    void save(Archive& archive) const {
        std::string outParent;
        archive(cereal::make_nvp("Parent", getPointOwningComponent().getName()));

        int i = 0;
        // serialize the incoming port
        std::pair<int, std::vector<std::string>> from(i++, getPortParentNameSeq(m_fromPort->getPort()));
        archive(cereal::make_nvp("From port", from));

        // serialize the outgoing, connecting ports
        std::map<int, std::vector<std::string>> idxToOutportNameSeq;
        std::map<PortPoint*, int> outportToIdx;
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
        std::map<PortPoint*, int> pointToIdx;
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
    ComponentGraphic* getPointOwningComponentGraphic() const;
    SimComponent& getPointOwningComponent() const;
    WirePoint* createWirePoint();
    void moveWirePoint(PortPoint* point, const QPointF scenePos);
    WireSegment* createSegment(PortPoint* start, PortPoint* end);
    void createRectilinearSegments(PortPoint* start, PortPoint* end);

    PortGraphic* m_fromPort = nullptr;
    std::vector<SimPort*> m_toPorts;
    std::vector<PortGraphic*> m_toGraphicPorts;
    std::set<WireSegment*> m_wires;
    std::set<WirePoint*> m_points;
};
}  // namespace vsrtl

#endif  // VSRTL_WIREGRAPHIC_H
