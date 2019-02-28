# VSRTL Reference

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