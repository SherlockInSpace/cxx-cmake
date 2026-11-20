# cxx-cmake
Template for building C++ projects/libraries/applications with CMake

> This README is being rewritten as the template manual; tracked in
> <https://github.com/SherlockInSpace/cxx-cmake/issues/39>.

## Building
Configure, build, and run the unit tests from the repository root:

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build
```

Tests build by default. Pass `-DBUILD_TESTING=OFF` at configure time to build
only the library.

Dependencies (OpenSSL, and GoogleTest for the tests) are declared through CPM
but resolve only through `find_package` by default, so configure fails rather
than download anything when one is missing. Pass `-DCPM_LOCAL_PACKAGES_ONLY=OFF`
to let CPM fetch what it cannot find locally (GoogleTest only; OpenSSL always
comes from the system), or `-DCPM_<Name>_SOURCE=/path/to/checkout` to build a
dependency from a local checkout.

To install the library, pass `-DCMAKE_INSTALL_PREFIX=<dir>` at configure time
and run `cmake --install build`.

## Documentation

[docs/DESIGN.md](docs/DESIGN.md) explains how the pieces fit together; [docs/DECISIONS.md](docs/DECISIONS.md)
explains why they are the way they are.
