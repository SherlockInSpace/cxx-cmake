# cxx-cmake
Template for building C++ projects/libraries/applications with CMake

## Instructions
Start off by setting your install directory location for the libraries to be built.

```
export INSTALL_DIR="$(pwd)/install"
```

## util library
To build the util library.

```
cd util
mkdir build
cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" -DCMAKE_PREFIX_PATH="${INSTALL_DIR}" -DBUILD_TEST="UNIT" ../
```