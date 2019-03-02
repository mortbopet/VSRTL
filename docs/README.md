# VSRTL Reference

## Usage
This repository provides a default application, [app.cpp](https://github.com/mortbopet/VSRTL/blob/master/app.cpp) which demonstrates the basic capabilities of VSRTL. Compiling and executing this app presents the VSRTL user interface, wherein the user may control the circuit, as described below.
<p align="center">
    <img src="https://raw.githubusercontent.com/mortbopet/VSRTL/master/resources/UI.png"/> 
</p>

Different circuits may be simulated by instatiating one of the various designs available in the VSRTL Components library, such as:
```c++
// app.cpp
int main(int argc, char** argv) {
    QApplication app(argc, argv);

    ${D} design; // ${D} =  vsrtl::RanNumGen or vsrtl::Counter<n> or others
    vsrtl::MainWindow w(design);

    w.show();

    app.exec();
}
```

It is the intention that VSRTL will evolve to be able to parse a HDL file (Verilog, VHDL, ...), and create a visual representation of this. However, currently, a "HDL-Like" syntax has been created for describing digital circuits which may be simulated using VSRTL.

Refer to [Docs:Core:Counter](https://github.com/mortbopet/VSRTL/blob/master/docs/core.md#example-counter) for an example implementation of a circuit in VSRTL syntax.

The VSRTL user-interface provides the following operations for controlling a simulation:

| <img src="https://raw.githubusercontent.com/mortbopet/VSRTL/master/graphics/resources/step.svg?sanitize=true" width="100pt"/> | <img src="https://raw.githubusercontent.com/mortbopet/VSRTL/master/graphics/resources/step-clock.svg?sanitize=true" width="100pt"/> | <img src="https://raw.githubusercontent.com/mortbopet/VSRTL/master/graphics/resources/rewind.svg?sanitize=true" width="100pt"/> | <img src="https://raw.githubusercontent.com/mortbopet/VSRTL/master/graphics/resources/reset.svg?sanitize=true" width="100pt"/>|
|:-:|:-:|:-:|:-:|
|Clock|Auto-clock|Rewind|Reset|
|Clock the circuit, storing all input values to registers as outputs, and propagate the remainder of the circuit| Issue the Clock action in regular intervals | Undo a clock cycle, rewinding the state of the circuit | Reset the circuit, setting all register output values to 0|


## Libraries
vsrtl is splsit into three libraries *core*, *graphics* and *components*.

### [VSRTL Core](core.md#top)
The core library provides all circuit primitives, such as multiplexers, registers and logic gates, which are required to describe larger, more complex circuits.
Furthermore, the core library contains logic for circuit propagation and clocking, used in controlling circuit simulation. Given a valid design, the core library exposes a graph of the described circuit which may be reset, as well as clocked and (**todo**) modified using testbenches.
The core library is fully independant on Qt, and as such simulations can be run without a graphical interface. The core library is designed in such a way as to allow for alternative visualizer implementations or command line interfaces.

### [VSRTL Graphics](graphics.md#top)
The *graphics* library is a Qt based library for displaying a graphical representation of a circuit described using vsrtl core. The graphics library implements graphics for each of the VSRTL Core components as well as component-specific visualizations. The aim of the visualizations is to display the underlying changes in the circuit.
Given a vsrtl::Design, the vsrtl::widget of the graphics library may traverse the graph which is the circuit design created in vsrtl core, and create a corresponding visualization of the circuit.

### [VSRTL Components](components.md#top)
The *components* library contains a collection of circuit designs such as:
- Exponenter
- Counter
- Random number generator