# cxx-cmake
Template for building C++ projects/libraries/applications with CMake

> This README is being rewritten as the template manual; tracked in
> <https://github.com/SherlockInSpace/cxx-cmake/issues/39>.

## Building
Configure, build, and run the unit tests from the repository root:

```
cmake -S . -B build -G Ninja -DBUILD_TEST=UNIT
cmake --build build
ctest --test-dir build
```

To install the library, pass `-DCMAKE_INSTALL_PREFIX=<dir>` at configure time
and run `cmake --install build`.

## Documentation

[docs/DESIGN.md](docs/DESIGN.md) explains how the pieces fit together; [docs/DECISIONS.md](docs/DECISIONS.md)
explains why they are the way they are.
