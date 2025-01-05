#ifndef VSRTL_WIREGRAPHIC_H
#define VSRTL_WIREGRAPHIC_H

#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbaseitem.h"
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

std::vector<std::string> getPortParentNameSeq(SimPort *p);
/**
 * @brief The PortPoint class
 * Base class for wire graphic points. This is the point type which is assigned
 * to PortGraphics. They are not moveable, but provide an interface between
 * moveable WirePoints (on WireSegments) and immovable PortPoints, on ports.
 */
class PortPoint : public QObject, public GraphicsBaseItem<QGraphicsItem> {
  Q_OBJECT
public:
  PortPoint(QGraphicsItem *parent);

  QPainterPath shape() const override;
  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item,
             QWidget *) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

  // Must be returned by copy; may get used to invalidate a wire, wherein each
  // output wire will get dereferenced with this point (and hence modifying
  // m_outputWires)
  std::vector<WireSegment *> getOutputWires() { return m_outputWires; }
  WireSegment *getInputWire() { return m_inputWire; }
  virtual const QPen &getPen();

  void addOutputWire(WireSegment *wire) { m_outputWires.push_back(wire); }
  void removeOutputWire(WireSegment *wire);
  void clearOutputWires() { m_outputWires.clear(); }
  void setInputWire(WireSegment *wire) { m_inputWire = wire; }
  void clearInputWire() { m_inputWire = nullptr; }

  void modulePositionHasChanged() override;

protected:
  WireSegment *m_inputWire = nullptr;
  WirePoint *m_draggedOnThis = nullptr;
  std::vector<WireSegment *> m_outputWires;

  // Cached shape and bounding rect
  QPainterPath m_shape;
  QRectF m_br;

private:
  void portPosChanged();

  PortGraphic *m_portParent = nullptr;
};

/**
 * @brief The WirePoint class
 * A point on a wire. Shall display a context menu to delete. is select- and
 * moveable.
 */
class WirePoint : public PortPoint {
public:
  WirePoint(WireGraphic *parent);

  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  const QPen &getPen() override;

  bool canMergeWith(WirePoint *point);
  void pointDrop(WirePoint *point);
  void pointDragEnter(WirePoint *point);
  void pointDragLeave(WirePoint *point);

private:
  WireGraphic *m_parent = nullptr;
};

class WireSegment : public QObject, public GraphicsBaseItem<QGraphicsItem> {
  Q_OBJECT
  friend class WirePoint;

public:
  WireSegment(WireGraphic *parent);

  void invalidate();
  bool isDrawn() const;
  bool isValid() const;
  void setStart(PortPoint *start);
  void setEnd(PortPoint *end);
  PortPoint *getStart() const { return m_start; }
  PortPoint *getEnd() const { return m_end; }
  QLineF getLine() const;

  /**
   * @brief geometryModified
   * Called whenever one of the lines end points has moved or changed. Prompts a
   * recalculation of the Wires geometry.
   */
  void geometryModified();

  QPainterPath shape() const override;
  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item,
             QWidget *) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

  QString deleted = "False";

private:
  PortPoint *m_start = nullptr;
  PortPoint *m_end = nullptr;
  WireGraphic *m_parent = nullptr;
  QLineF m_cachedLine;
  QPainterPath m_cachedShape;
  QRectF m_cachedBoundingRect;
};

class WireGraphic : public QObject, public GraphicsBaseItem<QGraphicsItem> {
  Q_OBJECT
  friend class PortGraphic;

public:
  enum class MergeType {
    CannotMerge,
    MergeSinkWithSource,
    MergeSourceWithSink,
    MergeParallelSinks
  };
  enum class WireType { BorderOutput, ComponentOutput };

  WireGraphic(ComponentGraphic *parent, PortGraphic *from,
              const std::vector<SimPort *> &to, WireType type);

  QRectF boundingRect() const override { return QRectF(); }
  const QPen &getPen();
  void postSceneConstructionInitialize1() override;
  void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {
  }

  void setWiresVisibleToPort(const PortPoint *p, bool visible);
  PortGraphic *getFromPort() const { return m_fromPort; }
  const std::vector<PortGraphic *> &getToPorts() const {
    return m_toGraphicPorts;
  }
  std::pair<WirePoint *, WireSegment *>
  createWirePointOnSeg(const QPointF scenePos, WireSegment *onSegment);
  void removeWirePoint(WirePoint *point);

  /**
   * @brief clearWirePoints
   * All wire points managed by this WireGraphic are deleted. As a result, the
   * only remaining WireSegment's will be between PortPoints.
   */
  void clearWirePoints();
  void clearWires();

  bool managesPoint(WirePoint *point) const;
  void mergePoints(WirePoint *base, WirePoint *toMerge);
  MergeType canMergePoints(WirePoint *base, WirePoint *toMerge) const;

  void clearLayout();

  /**
   * @brief postSerializeInit
   * Called after a WireGraphic has been loaded through serialization. This will
   * call the in- and output ports of the WireGraphic and re-execute their
   * visibility state functions, to propagate their visibility state to the wire
   * segments, post-serialization.
   */
  void postSerializeInit();

  template <class Archive>
  void load(Archive &archive) {
    prepareGeometryChange();
    setSerializing(true);
    // Deserialize the layout

    // This layout might originate from a similar component, but with a
    // different name. Get the serialized parent name as a reference for what
    // must be exchanged with the current parent name
    std::string inParent;
    archive(cereal::make_nvp("Parent", inParent));

    std::pair<int, std::vector<std::string>> from;
    archive(cereal::make_nvp("From port", from));
    std::replace(from.second.begin(), from.second.end(), inParent,
                 getParentComponent()->getName());

    std::map<int, std::vector<std::string>> idxToOutportNameSeq;
    archive(cereal::make_nvp("To ports", idxToOutportNameSeq));
    for (auto &iter : idxToOutportNameSeq) {
      std::replace(iter.second.begin(), iter.second.end(), inParent,
                   getParentComponent()->getName());
    }

    std::map<int, QPoint> idxToWirePointPos;
    archive(cereal::make_nvp("points", idxToWirePointPos));

    std::vector<std::pair<int, int>> wires;
    archive(cereal::make_nvp("wires", wires));

    // Clear current layout
    clearWirePoints();
    clearWires();

    std::map<int, PortPoint *> idxToPointPtr;
    idxToPointPtr[from.first] =
        m_fromPort->getPortPoint(vsrtl::SimPort::PortType::out);

    // Locate output ports from the layout indicies
    for (const auto &iter : idxToOutportNameSeq) {
      Q_ASSERT(idxToPointPtr.count(iter.first) == 0);
      PortPoint *point = nullptr;
      for (const auto &p : m_toGraphicPorts) {
        const auto nameSeq = getPortParentNameSeq(p->getPort());
        if (nameSeq == iter.second) {
          point = p->getPortPoint(vsrtl::SimPort::PortType::in);
          idxToPointPtr[iter.first] = point;
          break;
        }
        // Point in serialized layout was not found in this layout, continue...
      }
    }

    // Construct PointGraphic's for intermediate points
    for (const auto &p : idxToWirePointPos) {
      Q_ASSERT(idxToPointPtr.count(p.first) == 0);
      idxToPointPtr[p.first] = createWirePoint();
    }

    // Construct wires denoted in the layout
    for (const auto &w : wires) {
      if (idxToPointPtr.count(w.first) == 0) {
        continue; // Wire start point not found
      }
      if (idxToPointPtr.count(w.second) == 0) {
        continue; // Wire end point not found
      }
      auto *fromPort = idxToPointPtr[w.first];
      auto *toPort = idxToPointPtr[w.second];

      createSegment(fromPort, toPort);
    }

    // It may be that not all ports of a wire was denoted in the layout (ie.
    // changes to the design which the layout is applied on has been made).
    // Construct all missing wires between the source port
    for (const auto &p : m_toGraphicPorts) {
      const auto iter = std::find_if(
          idxToPointPtr.begin(), idxToPointPtr.end(), [=](const auto &itp) {
            return itp.second == p->getPortPoint(vsrtl::SimPort::PortType::in);
          });
      if (iter == idxToPointPtr.end()) {
        createSegment(m_fromPort->getPortPoint(vsrtl::SimPort::PortType::out),
                      p->getPortPoint(vsrtl::SimPort::PortType::in));
      }
    }

    // Move wire points (must be done >after< the point has been associated with
    // wires)
    for (const auto &p : idxToWirePointPos) {
      idxToPointPtr[p.first]->setPos(p.second);
    }

    setSerializing(false);
    postSerializeInit();
  }

  template <class Archive>
  void save(Archive &archive) const {
    std::string outParent;
    archive(cereal::make_nvp("Parent", getParentComponent()->getName()));

    int i = 0;
    // serialize the incoming port
    std::pair<int, std::vector<std::string>> from(
        i++, getPortParentNameSeq(m_fromPort->getPort()));
    archive(cereal::make_nvp("From port", from));

    // serialize the outgoing, connecting ports
    std::map<int, std::vector<std::string>> idxToOutportNameSeq;
    std::map<PortPoint *, int> outportToIdx;
    for (const auto &p : m_toGraphicPorts) {
      // @todo: this is not sufficient, ports may be named identically.
      // It should be a hierarchical list including its parent components
      idxToOutportNameSeq[i] = getPortParentNameSeq(p->getPort());
      outportToIdx[p->getPortPoint(vsrtl::SimPort::PortType::in)] = i;
      i++;
    }
    archive(cereal::make_nvp("To ports", idxToOutportNameSeq));

    // Each point managed by this wire is enumerated and associated with its
    // position
    std::map<int, QPoint> idxToPos;
    std::map<PortPoint *, int> pointToIdx;
    for (const auto &p : m_points) {
      idxToPos[i] = p->pos().toPoint();
      pointToIdx[p] = i;
      i++;
    }
    archive(cereal::make_nvp("points", idxToPos));

    // Each wire segment will connect to either a point or a port
    std::vector<std::pair<int, int>> wires;
    for (auto &w : m_wires) {
      auto *start = w->getStart();
      auto *end = w->getEnd();

      int startIdx = 0;
      int endIdx = 0;

      if (m_fromPort->getPortPoint(vsrtl::SimPort::PortType::out) == start) {
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
  SimComponent *getParentComponent() const;
  WirePoint *createWirePoint();
  void moveWirePoint(PortPoint *point, const QPointF scenePos);
  WireSegment *createSegment(PortPoint *start, PortPoint *end);
  void createRectilinearSegments(PortPoint *start, PortPoint *end);

  ComponentGraphic *m_parent = nullptr;
  PortGraphic *m_fromPort = nullptr;
  std::vector<SimPort *> m_toPorts;
  std::vector<PortGraphic *> m_toGraphicPorts;
  std::set<WireSegment *> m_wires;
  std::set<WirePoint *> m_points;
  WireType m_type;
};
} // namespace vsrtl

#endif // VSRTL_WIREGRAPHIC_H
