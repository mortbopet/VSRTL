# VSRTL
***V**isual **S**imulation of **R**egister **T**ransfer **L**ogic*

vsrtl is a collection of libraries and applications for visualizing simulations of digital circuits.

vsrtl is intended to be an intuitive HDL simulator which may be used when teaching about digital circuits and boolean logic. 

<p align="center">
  <img src="https://github.com/mortbopet/vsrtl/blob/master/resources/gif1.gif?raw=true" width=75%/>
</p>
Figure: A simulation of a 3-bit counter utilizing 3 full adders. **Note**; placement and routing have not yet been implemented, and as such component placement in this image is manual, and signals are drawn as straight lines from source to sinks.

# Libraries
vsrtl is split into three libraries *core*, *graphics* and *components*.

## VSRTL Core
The core library is **fully independant** on Qt, and as such simulations can be run without a graphical interface. The core library is designed in such a way as to allow for alternative visualizer implementations or command line interfaces.
The core library provides all circuit primitives, such as multiplexers, registers and logic gates, which are required to describe larger, more complex circuits.
Furthermore, the core library contains logic for circuit propagation and clocking, used in controlling circuit simulation. Given a valid design, the core library exposes a graph of the described circuit which may be reset, as well as clocked and (**todo**) modified using testbenches.

## VSRTL Graphics
The graphics library is a Qt based library for displaying a graphical representation of a circuit described using vsrtl core. The graphics library implements graphics for each of the VSRTL Core components as well as component-specific visualizations. The aim of the visualizations is to display the underlying changes in the circuit.
Given a vsrtl::Design, the vsrtl::widget of the graphics library may traverse the graph which is the circuit design created in vsrtl core, and create a corresponding visualization of the circuit.

## VSRTL Components
The components library contains a collection of circuit designs, which utilize the Core library. Examples are:
- Exponenters
- Counters
- Random number generator

# Building
With the dependencies installed, open `vsrtl/CMakeLists.txt` as a CMake project in your favourite editor, run CMake and build `app.cpp`.
## Dependencies:
* **Core**
 * C++14 toolchain
 * CMake
* **Graphics**
* Qt 5.10+: https://www.qt.io/download
