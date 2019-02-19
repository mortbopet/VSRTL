# vsrtl
Visual Simulation of Register Transfer Logic


# Todo:
- Waveform generation and dump (VCD)
- Netlist delegates - allow for modifying register values on the fly. This is a strength of the program!

# Libraries

## VSRTL Core

The core library is fully independant on Qt, and as such simulations can be run without a graphical interface.

Todo:
- CLI for Core

## VSRTL Graphics

## VSRTL Components
Collection of predefined components. These are simple building blocks which may be used in other designs, such as:

- Constants
- ALU
- Logic gates

# Building
## Dependencies:

### Core:
* C++14 toolchain
* CMake

### Graphics:
* Qt 5.10+: https://www.qt.io/download
