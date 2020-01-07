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

public slots:
    void clock();
    void reset();
    void rewind();
    void setOutputPortValuesVisible(bool visible);

    // Selections which are imposed on the scene from external objects (ie. selecting items in the netlist)
    void handleSelectionChanged(const std::vector<SimComponent*>& selected, std::vector<SimComponent*>& deselected);

signals:
    void canrewind(bool);
    void componentSelectionChanged(const std::vector<SimComponent*>&);
    void portSelectionChanged(const std::vector<SimPort*>&);

private slots:
    void handleSceneSelectionChanged();

private:
    void checkCanRewind();

    // State variable for reducing the number of emitted canrewind signals
    bool m_designCanrewind = false;

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
