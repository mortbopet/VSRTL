# VSRTL Graphics

- [VSRTL Graphics](#vsrtl-graphics)
  - [VSRTL Core circuit traversal](#vsrtl-core-circuit-traversal)
  - [Place & Route](#place--route)
  - [Graph Traversal](#graph-traversal)

## VSRTL Core circuit traversal

## Place & Route

## Graph Traversal
All graphics objects which correspond to a similar VSRTL core component will have access to the core component through a member pointer. With access to the underlying Core component, the techniques presented in [core graphic traversal](https://github.com/mortbopet/vsrtl/blob/master/docs/core.md#traversing-the-graph) may be applicable. 

`ComponentGraphic` objects are created with a parent/child relationship given the parent type of `QGraphicsItem`. The parent `ComponentGraphic` of a `ComponentGraphic` may be accessed through `ComponentGraphic::getParent`. Subcomponents of a `ComponentGraphic` may be accessed through the member variable `m_subcomponents`, a mapping between the subcomponent `ComponentGraphic`s and the corresponding `Component`.