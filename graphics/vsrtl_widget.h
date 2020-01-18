#ifndef VSRTL_WIDGET_H
#define VSRTL_WIDGET_H

#include <QMainWindow>
#include "vsrtl_componentgraphic.h"
#include "vsrtl_portgraphic.h"

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
    explicit VSRTLWidget(QWidget* parent = nullptr);
    ~VSRTLWidget();

    void addComponent(ComponentGraphic* g);
    void expandAllComponents(ComponentGraphic* fromThis = nullptr);

    void setDesign(SimDesign* design);
    void clearDesign();
    bool isReversible();

public slots:
    void run();
    void stop() { m_stop = true; }
    void clock();
    void reset();
    void reverse();
    void setOutputPortValuesVisible(bool visible);

    // Selections which are imposed on the scene from external objects (ie. selecting items in the netlist)
    void handleSelectionChanged(const std::vector<SimComponent*>& selected, std::vector<SimComponent*>& deselected);

signals:
    void canReverse(bool);
    void componentSelectionChanged(const std::vector<SimComponent*>&);
    void portSelectionChanged(const std::vector<SimPort*>&);

private slots:
    void handleSceneSelectionChanged();

private:
    // State variable for reducing the number of emitted canReverse signals
    bool m_designCanreverse = false;

    bool m_stop = false;

    void registerShapes() const;

    void initializeDesign();
    Ui::VSRTLWidget* ui;

    vsrtl::ComponentGraphic* m_topLevelComponent = nullptr;

    VSRTLView* m_view;
    VSRTLScene* m_scene;

    SimDesign* m_design;
};

}  // namespace vsrtl

#endif  // VSRTL_WIDGET_H
