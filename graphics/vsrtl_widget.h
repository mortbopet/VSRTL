#ifndef VSRTL_WIDGET_H
#define VSRTL_WIDGET_H

#include <QMainWindow>
#include "vsrtl_componentgraphic.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace vsrtl {

class VSRTLView;
class VSRTLScene;

namespace Ui {
class VSRTLWidget;
}

class Design;

class VSRTLWidget : public QWidget {
    Q_OBJECT

public:
    explicit VSRTLWidget(Design& arch, QWidget* parent = nullptr);
    ~VSRTLWidget();

    void addComponent(ComponentGraphic* g);
    void expandAllComponents(ComponentGraphic* fromThis = nullptr);

public slots:
    void clock();
    void reset();
    void rewind();

    // Selections which are imposed on the scene from external objects (ie. selecting items in the netlist)
    void handleSelectionChanged(const std::vector<Component*>& selected, std::vector<Component*>& deselected);

signals:
    void canrewind(bool);
    void componentSelectionChanged(const std::vector<Component*>&);
    void portSelectionChanged(const std::vector<PortBase*>&);

private slots:
    void handleSceneSelectionChanged();

private:
    void checkCanRewind();

    // State variable for reducing the number of emitted canrewind signals
    bool m_designCanrewind = false;

    void registerShapes() const;

    void initializeDesign(Design& arch);
    Ui::VSRTLWidget* ui;

    vsrtl::ComponentGraphic* m_topLevelComponent = nullptr;

    VSRTLView* m_view;
    VSRTLScene* m_scene;

    Design& m_arch;
};

}  // namespace vsrtl

#endif  // VSRTL_WIDGET_H
