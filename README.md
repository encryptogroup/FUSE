### Repository Structure
* `.github/workflows`: Contains configuration of Continuous Integration (CI) pipeline
* `extern`: Other Repositories used in this implementation, currently consisting of:
  * [FlatBuffers](https://github.com/google/flatbuffers)
  * [HyCC](https://gitlab.com/securityengineering/HyCC)
  * [MOTION](https://github.com/encryptogroup/MOTION)
  * [MP-SPDZ](https://github.com/data61/MP-SPDZ)
  * [Google Benchmark](https://github.com/google/benchmark)
  * [Gzip Header-Only C++ Library](https://github.com/alphamarket/gzip-hpp)
* `examples`: Contains example circuits in
  * [Bristol Format](https://homes.esat.kuleuven.be/~nsmart/MPC/old-circuits.html) in `examples/bristol_circuits`
  * HyCC's [`.circ` format](https://gitlab.com/securityengineering/HyCC/-/blob/master/src/libcircuit/simple_circuit_file.cpp) in `examples/hycc_circuits` after downloading them from [TUdatalib](https://tudatalib.ulb.tu-darmstadt.de/handle/tudatalib/3776)
* `fbs`: FlatBuffers schemas
* `src`: C++ Source files
  * `src/backend`: C++ header and source files to translate FUSE IR to other representations or to execute with Secure Multiparty Computation backends
  * `src/core`: C++ utility to build and use FUSE IR in frontends, backends, or passes
  * `src/examples`: Contains small example code snippets to show how to use FUSE IR
  * `src/frontend`: Translate other formats to FUSE IR, currently being Bristol Format and the `.circ` format
  * `src/passes`: Contains analyses and optimizations for FUSE IR
* `tests`: Contains googletest tests for the source code
* `benchmarks`: Contains source code for the benchmarks in our paper

## Setup
TL;DR -- Install flatbuffers locally, then compile FUSE
```
python3 setup.py -sb
```
Please install the project dependencies and HyCC frontend dependencies beforehand.

### Project Dependencies
To build FUSE successfully, you will need the following the packages:
```
apt-get install build-essential software-properties-common cmake libssl-dev libgoogle-perftools-dev libboost-all-dev libz-dev curl git-lfs
```
Also, you will need a C++ compiler with support for C++-20 to be able to compile this framework, e.g., `gcc-11` or newer.


#### HyCC Frontend Dependencies
If you want to use the HyCC Frontend for FUSE IR, you will need additional setup:
```
apt-get install libwww-perl
apt-get clean && apt-get update && apt-get install -y locales
locale-gen en_US.UTF-8
apt-get install flex bison default-jdk
```

#### Troubleshooting CMake when installing the MOTION Backend
As both FUSE and MOTION use FlatBuffers in their implementation, both projects have CMake dependencies on the FlatBuffers project. CMake has troubles with such dependency situations, so it may produce errors indicating that the FlatBuffers library is being imported twice or not at all. In that case, you will have to compile and install FlatBuffers from sources locally:
```
cd extern/flatbuffers
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake -B build
cmake --build build
cd build && make install
```


### How to Build FUSE
Clone this repository
```
git clone --recurse-submodules https://github.com/encryptogroup/FUSE.git
```

Then, use CMake to build this project:
```
cmake -B build
cmake --build build
```

If you want to run the Developer Tutorial:
```
cd build/bin
./fuse_dev_tutorial
```

### Citing this work
The design of FUSE is presented in this [paper](https://eprint.iacr.org/2023/563). If you want to use FUSE for your academic projects, please cite
```
@inproceedings{fuse,
    author = {Lennart Braun and Moritz Huppert and Nora Khayata and Thomas Schneider and Oleksandr Tkachenko},
    title = {{FUSE} -- {F}lexible File Format and Intermediate Representation for Secure Multi-Party Computation},
    booktitle = {AsiaCCS},
    year = {2023},
}
```

### Example Circuits
For benchmarking and testing, we have used example circuits in [Bristol Format](https://homes.esat.kuleuven.be/~nsmart/MPC/old-circuits.html) and [HyCC](https://gitlab.com/securityengineering/HyCC) binary format. We have decided to only keep the Bristol format examples for easy access inside the repostory, to not blow up the size of this repository. However, there is a public ZIP-file containing both Bristol Format and HyCC circuits available in the [TUdatalib](https://tudatalib.ulb.tu-darmstadt.de/handle/tudatalib/3776). It is sufficient to unpack the ZIP-file and move the folder `examples/hycc-circuits` to `FUSE/examples/hycc-circuits`.
