# VSRTL
[![Build Status](https://travis-ci.org/mortbopet/VSRTL.svg?branch=master)](https://travis-ci.org/mortbopet/VSRTL)  
***V**isual **S**imulation of **R**egister **T**ransfer **L**ogic*

VSRTL is a collection of libraries and applications for visualizing simulations of digital circuits.
VSRTL is intended to be an intuitive HDL simulator which may be used for teaching digital circuits and boolean logic. 

Refer to the [reference section](docs/README.md) for implementation and usage documentation.  
If you would like to contribute, check the [issues](https://github.com/mortbopet/vsrtl/issues) section - There's plenty of work to be done!

<p align="center">
  <img src="https://github.com/mortbopet/vsrtl/blob/master/resources/gif1.gif?raw=true" width=75%/>
</p>

Figure: A simulation of a 3-bit counter utilizing 3 full adders. **Note**; placement and routing have not yet been implemented, and as such component placement in this image is manual, and signals are drawn as straight lines from source to sinks.


# Building
With the dependencies installed, open `vsrtl/CMakeLists.txt` as a CMake project in your favourite editor, run CMake and build `app.cpp`.
## Dependencies:
* **Core**
  * C++14 toolchain
  * CMake
* **Graphics**
  * Qt 5.9.3+: https://www.qt.io/download
