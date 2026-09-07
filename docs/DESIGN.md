# Design

Overview of how the four repos relate. Rationale is in [DECISIONS.md](DECISIONS.md), not repeated
here.

## Repos

- `cxx-cmake-container` builds the `ci`, `dev` and `yocto` images. CI and the devcontainer in the
  other repos use them (see the `container:` key in the workflows).
- `cxx-cmake-library` is the library template. `util` is a Bloom filter hashed with OpenSSL, a
  vendored thread pool and a header-only `bit` module. Replace `src/` and `include/util/`; everything else
  (install, tests, CI, release) is the reusable part.
- `cxx-cmake-app` is the application template. Same layout, links the installed `util`, plus one
  external dep (spdlog), a pytest integration tier and an optional Tracy build. Its CI calls the
  library's workflows.
- `meta-cxx-cmake` is the Yocto layer. Recipes for `util` and the app, a kas config, ptest packaging
  and the SDK job. The `yocto` image is built from that SDK.

container -> library -> app. meta-cxx-cmake packages library and app for the board.

## Container

One Dockerfile, two stages:

- `ci`: toolchain, baseline libraries, quality tools. Runs as root for GitHub Actions.
- `dev`: `ci` plus a non-root `dev` user, sudo and zsh. Pass `HOST_UID`/`HOST_GID` and the
  entrypoint remaps the user so files under `/work` are yours.

Versions track Wrynose 6.0.2. Ubuntu 26.04 is a patch level behind on three of them.

| Component  | Wrynose 6.0.2 | Image                   |
|------------|---------------|-------------------------|
| gcc/g++    | 15.3.0        | 15.2.0                  |
| glibc      | 2.43          | 2.43                    |
| binutils   | 2.46.1        | 2.46                    |
| CMake      | 4.3.1         | 4.3.1 (Kitware tarball) |
| Ninja      | 1.13.2        | 1.13.2                  |
| LLVM/clang | 22.1.2        | 22.1.2                  |
| OpenSSL    | 3.5.7         | 3.5.5                   |
| GoogleTest | 1.17.0        | 1.17.0                  |

One semver for all three images, no `latest`. CI and the devcontainer use the digest.

```sh
docker run --rm -it -e HOST_UID=$(id -u) -e HOST_GID=$(id -g) \
  -v "$PWD":/work -v cxx-cmake-ccache:/home/dev/.ccache \
  ghcr.io/sherlockinspace/cxx-cmake-container/dev:X.Y.Z@sha256:<digest>
```

Run it from the directory above your checkouts, then `CPM_util_SOURCE=/work/cxx-cmake-library`
can point at a sibling checkout.

## Install layout

After `cmake --install`:

```
<prefix>/include/util/*.hpp
<prefix>/lib/libutil.so  ->  libutil.so.X  ->  libutil.so.X.Y.Z
<prefix>/lib/cmake/util/utilConfig.cmake  utilConfigVersion.cmake  utilTargets*.cmake
```

The exported target is `util::util`, same name in-tree. A consumer needs two lines:

```cmake
find_package(util CONFIG REQUIRED)
target_link_libraries(app PRIVATE util::util)
```

The recipe packages the same layout as `util-dev`.

## Dependencies

CPM with `CPM_USE_LOCAL_PACKAGES=ON` and `CPM_LOCAL_PACKAGES_ONLY=ON` by default: `find_package`
from the image or the Yocto sysroot, or configure fails. Nothing is downloaded unless you set
`CPM_LOCAL_PACKAGES_ONLY=OFF`. `CPM_<NAME>_SOURCE=/path` points one dependency at a local checkout.

## Dev loops

- `cmake --preset dev && cmake --build --preset dev && ctest --preset dev` in the container, on
  x86-64 or arm64.
- `kas build --target util` through `kas-container` when you change the install layout or the
  layer. `EXTERNALSRC:pn-util = "/work/cxx-cmake-library"` builds your checkout without pushing.
- `cmake --preset target` in the `yocto` image cross-compiles for the board (see below).

## CI

| Tier       | When                     | Answers                                          |
|------------|--------------------------|--------------------------------------------------|
| PR gate    | every pull request       | did this change break anything?                  |
| post-merge | every push to `main`     | does the full matrix pass, and which merge broke it? |
| weekly     | cron, no new commits     | did the environment drift?                       |
| release    | a release being created  | do the benchmarks hold against the last release? |

## Releases

Commit subjects on `main` decide the bump: `fix:` is a patch, `feat:` a minor, `!` a major.
release-please keeps a rolling PR that updates `CHANGELOG.md` and the `project(VERSION ...)` line.
Merging it tags `vX.Y.Z` and creates the GitHub Release. Each repo releases this way. A container
release publishes the images. A library release runs the benchmark gate, and meta-cxx-cmake then
bumps `SRCREV` to the new tag.

## Board + SDK

A self-hosted runner on the board, or on a host that can reach it, runs `ptest-runner util` on
dispatch. Adding a board means adding its `MACHINE` next to `qemuarm64` in the kas config. The
layer's SDK job publishes the installers and the `yocto` image unpacks them, so `cmake --preset
target` in that image finds `util` in the sysroot and cross-compiles for the board. What it builds
runs only on the board.
