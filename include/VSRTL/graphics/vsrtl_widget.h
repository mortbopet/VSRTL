#ifndef VSRTL_WIDGET_H
#define VSRTL_WIDGET_H

#include "vsrtl_componentgraphic.h"
#include "vsrtl_portgraphic.h"
#include <QMainWindow>

#include <QtConcurrent/QtConcurrent>

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace vsrtl {

class VSRTLView;
class VSRTLScene;

namespace Ui {
class VSRTLWidget;
}

class VSRTLWidget : public QWidget {
  Q_OBJECT

public:
  explicit VSRTLWidget(QWidget *parent = nullptr);
  ~VSRTLWidget();

  void addComponent(ComponentGraphic *g);
  void expandAllComponents(ComponentGraphic *fromThis = nullptr);
  ComponentGraphic *getTopLevelComponent() { return m_topLevelComponent; }

  void setDesign(SimDesign *design, bool doPlaceAndRoute = false);
  void clearDesign();
  bool isReversible();

  void setOutputPortValuesVisible(bool visible);
  void setDarkmode(bool enabled);
  void setLocked(bool locked);
  void zoomToFit();

  /// Called whenever the state of the simulator and the visualization is out of
  /// sync, i.e., after running the processor. Updates the scene and all text
  /// items within it to reflect the current state of the processor.
  void sync();

public slots:

  /**
   * @brief run
   * Asynchronously run the design until m_stop is asserted. @returns a future
   * which may be watched to monitor run finishing. Additionally, a functor
   * @param cycleFunctor can be passed to the function. This functor will be
   * executed after each clock cycle. Example uses of such functor could be; ie.
   * if simulating a processor, whether the prcoessor has hit a breakpoint and
   * running needs to be terminated.
   */
  QFuture<void>
  run(const std::function<void()> &cycleFunctor = std::function<void()>());
  void stop() { m_stop = true; }
  void clock();
  void reset();
  void reverse();

  // Selections which are imposed on the scene from external objects (ie.
  // selecting items in the netlist)
  void handleSelectionChanged(const std::vector<SimComponent *> &selected,
                              const std::vector<SimComponent *> &deselected);

signals:
  void runFinished();

private slots:

signals:
  void canReverse(bool);
  void componentSelectionChanged(const std::vector<SimComponent *> &);
  void portSelectionChanged(const std::vector<SimPort *> &);

private slots:
  void handleSceneSelectionChanged();

private:
  // State variable for reducing the number of emitted canReverse signals
  bool m_designCanreverse = false;

  std::atomic<bool> m_stop = false;

  void initializeDesign(bool doPlaceAndRoute);
  Ui::VSRTLWidget *ui;

  ComponentGraphic *m_topLevelComponent = nullptr;

  VSRTLView *m_view;
  VSRTLScene *m_scene;

  SimDesign *m_design = nullptr;
};

} // namespace vsrtl

#endif // VSRTL_WIDGET_H
