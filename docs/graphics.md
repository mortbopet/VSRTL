# VSRTL Graphics

- [VSRTL Graphics](#vsrtl-graphics)
  - [Place & Route](#place--route)
  - [Graph Traversal](#graph-traversal)

## Place & Route

## Graph Traversal
All graphics objects which correspond to a similar VSRTL Core component will have access to its paired Core component through a member pointer. With access to the underlying Core component, the techniques presented in [Core graph traversal](https://github.com/mortbopet/vsrtl/blob/master/docs/core.md#traversing-the-graph) may be applicable. 
All graphics objects register themselves with their Core component through the `vsrtl::Base` class functions. When the graph has been traversed in the Core layer, the corresponding Graphics object for a component may be accessed as follows:
```C++
Register* reg = ...;
ComponentGraphic* reg_graphic = static_cast<ComponentGraphic*>(reg->getGraphic());

Component* multiplexer = ...;
MultiplexerGraphic* mux_graphic = static_cast<MultiplexerGraphic>(multiplexer->getGraphic());

Port* port = ...;
PortGraphic* port_graphic = static_cast<PortGraphic*>(port->getGraphic());
```
From here on, physical properties such as the dimensions of a component, port placements etc. may be gathered.

**Note**: This method requires the knowledge of which Graphics object type corresponds to which Core object types.