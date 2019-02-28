# VSRTL Core

- [VSRTL Core](#vsrtl-core)
  - [Components](#components)
  - [Design](#design)
  - [Ports](#ports)
- [Circuit Graph Structure](#circuit-graph-structure)
  - [Traversing the graph](#traversing-the-graph)
- [Inner workings](#inner-workings)
  - [Circuit verification](#circuit-verification)
  - [Propagation algorithm](#propagation-algorithm)
  - [Example: Counter](#example-counter)

The following sections refer to classes available in the VSRTL core library.

`code` refers to in-source classes, member variables and functions.


## Components
`Component`s are the building block of the graph which represents a circuit in vsrtl. A component may have an arbitrary number of input- and output `Port`s, which may be assigned to by the `Component`.

## Design
The `Design` class is comparable to the "top" file in a HDL project. Through a `Design`, a circuit may be clocked and reset. 
The graph which represents the circuit is owned by the `Design` and has its lifecycle managed by the lifecycle of the `Design`.
A `Design` is a subclass of the `Component` class, and as such all components within the `Design` is present in the `m_subcomponents` variable.

## Ports

A port may only have one input (source) but may have multiple outputs (sinks). Ports connect to other ports.
A port contains a `width` value. This value corresponds to the bit width of the value which the port represents. A port must have its width set at construction time or before connecting the port. During connection of components, port widths are compared, verifying that ports are of equal width.

# Circuit Graph Structure
<p align="center">
  <img src="https://github.com/mortbopet/VSRTL/blob/master/resources/graphstructure.png?raw=true" width=75%/>
</p>

The graph structure of a circuit in VSRTL may be traversed as a bidirectional or a unidirectional graph, with verticies of types `Component` and `Port`. A `Component` may have subcomponents, 

## Traversing the graph
Traversal of the graph is done on 

`Component::getInputComponents` and `Component::getOutputComponents` returns a `std::vector` of the connected components of a component. While a `std::set` would be natural to return, a `std::vector` is utilized, allowing for duplicate connections between components, say, if the following structure is present:
```
___    ___
|  |-> |  |
|__|-> |__|
```
Wherein the multiple number of edges between two components is valuable information for graph partitioning algorithms, used within VSRTL Graphics.
Both of the aforementioned functions generates the in- and output components by querying the in- and output ports of the current component, locating the sources and sinks of these ports, and from these source and sink ports, return their parent components.


# Inner workings

## Circuit verification
For a circuit to be considered correct and simulateable, the following conditions must evaluate to true:
* **Combinational loops**
  * During circuit verification, the graph is traversed from sources to sinks, with `Register`s being seen as a cut in the graph. If cycles are detected, this is a sign of a combinational loop in the circuit, yielding the circuit invalid.
* **Port verification**
  * Input ports must be connected to the output port of another component. If input ports may be disregarded for a component, similarly to HDL designs, the input port should be tied off to a constant value. In VSRTL this corresponds to connecting an input port to the output port of a constant component.
  * Ports must have their width set before connecting a port to other ports.

## Propagation algorithm
In view of the propagation algorithm, the graph is traversed as a unidirectional graph, with `Register` being a cut in the graph. During `Design` circuit verification, the graph is analyzed for cycles, and if so, this is an indication of a combinational loop.

Components with no input ports are considered to be constant components, which are not considered for circuit propagation, except for the first clock cycle. 



## Example: Counter
A counting circuit may be represented by joining together a string of [full adder circuits](https://en.wikipedia.org/wiki/Adder_(electronics)#Full_adder). `n` full adders represents an `n` bit counter. 
Initially, a full adder component must be created;
```c++
class FullAdder : public Component {
public:
    FullAdder(std::string name) : Component(name) {
        A >> *xor1->in[0];
        B >> *xor1->in[1];

        xor1->out >> *xor2->in[0];
        Cin >> *xor2->in[1];

        xor1->out >> *and1->in[0];
        Cin >> *and1->in[1];

        A >> *and2->in[0];
        B >> *and2->in[1];

        and1->out >> *or1->in[0];
        and2->out >> *or1->in[1];

        or1->out >> Cout;
        xor2->out >> S;
    }

    INPUTPORT_W(A, 1);
    INPUTPORT_W(B, 1);
    INPUTPORT_W(Cin, 1);

    OUTPUTPORT_W(S, 1);
    OUTPUTPORT_W(Cout, 1);

    SUBCOMPONENT(xor1, Xor, 2, 1);
    SUBCOMPONENT(xor2, Xor, 2, 1);
    SUBCOMPONENT(and1, And, 2, 1);
    SUBCOMPONENT(and2, And, 2, 1);
    SUBCOMPONENT(or1, Or, 2, 1);

};
```
Outside of the constructor, the subcomponents of the full adder are specified;
`SUBCOMPONENT(xor1, Xor, 2, 1)` instantiates an xor-gate (of the class `Xor`), named xor1, which has 2 inputs and a bit width of 1.
`INPUTPORT_W(A, 1)` instantiates an inport port named A, with a bit width of 1.

With the subcomponents, as well as the input- and output ports of the full adder specified, the internals of the full adder must be connected.
Connections are facilitated by the `>>` operator. This operator expects two `Port&` arguments, wherein the left-hand side represents the source of the signal, and the right hand side the sink of the signal.
As seen, an `INPUTPORT` may be used as a source for ports within the `FullAdder` component.
`A >> *xor1->in[0]` connects input port A to the first input of the xor1 gate. the `>>` operator
`or1->out >> Cout` connects the output of the or1 component with the `Cout` port. 

Having specified and connected a full-adder component, it is time to utilize this component in a `Counter` circuit;
```c++
template <unsigned int width>
class Counter : public Design {
public:
    Counter() : Design(std::to_string(width) + " bit counter") {
        for (int i = 0; i < width; i++) {
            adders.push_back(create_component<FullAdder>(this, "adder_" + std::to_string(i)));
            regs.push_back(create_component<Register>(this, "reg_" + std::to_string(i), 1));
        }

        // Connect
        c0->out >> adders[0]->Cin;
        c1->out >> adders[0]->A;
        regs[0]->out >> adders[0]->B;
        regs[0]->out >> *value->in[0];
        adders[0]->S >> regs[0]->in;

        for (int i = 1; i < width; i++) {
            adders[i - 1]->Cout >> adders[i]->Cin;
            regs[i]->out >> adders[i]->A;
            regs[i]->out >> *value->in[i];
            c0->out >> adders[i]->B;
            adders[i]->S >> regs[i]->in;
        }
    }

    std::vector<FullAdder*> adders;
    std::vector<Register*> regs;

    SUBCOMPONENT(value, Collator, width);

    SUBCOMPONENT(c0, Constant, 0, 1);
    SUBCOMPONENT(c1, Constant, 1, 1);
};
```
A `std::vector` of `FullAdder`s and `Register`s are specified as member variables, along with a couple of 1-bit wide constants of values 0 and 1.
Furthermore, `SUBCOMPONENT(value, Collator, width)` creates a Collator component, of `width` size. A collator joins together a number of 1-bit wide signals into an output signal of `width` width.

Within the constructor, #`width` adders and registers are created, and subsequently wired up. 
As seen, the `Counter` component is an instance of a `Design` and as such may be used for simulation. 

in `app.cpp` the Counter may be instantiated with the required `width` and parsed as an argument to the `vsrtl::MainWindow` class - from here on out, the Graphics library will traverse the circuit and create the corresponding graphical representation
```c++
int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(icons);

    VSRTL::Counter<8> design;
    VSRTL::MainWindow w(design);

    w.show();

    app.exec();
}
```