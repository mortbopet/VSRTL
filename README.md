# VSRTL
[![VSRTL build and test](https://github.com/mortbopet/VSRTL/actions/workflows/build-and-test.yml/badge.svg?branch=master)](https://github.com/mortbopet/VSRTL/actions/workflows/build-and-test.yml)
[![Gitter](https://badges.gitter.im/Ripes-VSRTL/VSRTL.svg)](https://gitter.im/Ripes-VSRTL/)

***V**isual **S**imulation of **R**egister **T**ransfer **L**ogic*

VSRTL is a framework for describing, visualizing and simulating digital circuits.  
A VSRTL-described circuit may be built and simulated as a standalone application or embedded within a Qt-based C++ application. As an example, VSRTL is used as the simulation and visualization framework for [Ripes](https://github.com/mortbopet/Ripes), a graphical processor simulator and assembly editor for the RISC-V ISA.

Refer to the [reference section](docs/README.md) for implementation and usage documentation.  
If you would like to contribute, check the [issues](https://github.com/mortbopet/vsrtl/issues) section - There's plenty of work to be done!  
For questions, comments, feature requests, or new ideas, don't hesitate to share these at [the discussions page](https://github.com/mortbopet/VSRTL/discussions).

<p align="center">
  <img src="https://github.com/mortbopet/vsrtl/blob/master/resources/gif1.gif?raw=true" width=75%/>
</p>

Figure: A visualization of [VelonaCore](https://github.com/mortbopet/VelonaCore), a single cycle processor implementing the [Leros instruction set](https://leros-dev.github.io).

# Building
```
git clone --recurse-submodules https://github.com/mortbopet/VSRTL.git
cd VSRTL/
cmake .
make -j$(nproc)
```

## Dependencies:
* **Core**
  * C++17 toolchain
  * CMake
* **Graphics**
  * Qt 6.5.0+: https://www.qt.io/download

---
In papers and reports, please refer to VSRTL as follows: 'Morten Borup Petersen. VSRTL. https://github.com/mortbopet/VSRTL', e.g. using the following BibTeX code:
```
@MISC{VSRTL,
	author = {Morten Borup Petersen},
	title = {VSRTL: Visual Simulation of Register Transfer Logic},
	howpublished = "\url{https://github.com/mortbopet/VSRTL}"
}
```
