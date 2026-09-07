# Design

How the four cxx-cmake repositories fit together, for someone who wants to use or extend the
template. The reasons behind the choices are in [DECISIONS.md](DECISIONS.md).

## The four repositories

```
cxx-cmake-container ─ ci / dev images ─▶ cxx-cmake-library ─ installed util ─▶ cxx-cmake-app
        │                                        │                                   │
        │ yocto image (from the SDK)             │ util recipe pins a release tag    │ app recipe
        ▼                                        ▼                                   ▼
              meta-cxx-cmake — Yocto layer pinned with kas: recipes, ptest packaging, SDK job
```

`cxx-cmake-library` is the library template: a small C++23 library called `util` (a Bloom filter
hashed through OpenSSL, a vendored thread pool and a header-only `bit` module). What you keep is the
install contract, the test tiers, the CI layout and the release flow. The code is there to be
replaced: start with "Use this template", rename `util` and delete the rest.

`cxx-cmake-app` is the application template. It consumes the installed `util` as a downstream
project would and adds spdlog as the third-party dependency example, a pytest integration tier and
an optional Tracy build. Its CI calls the library's workflows.

`cxx-cmake-container` builds the images everything else builds inside: one Dockerfile with two
stages, a smoke test for the toolchain and a release workflow publishing to GHCR.

`meta-cxx-cmake` is the Yocto layer: a recipe for `util` and one for the app, a kas configuration,
ptest packaging and the SDK job. kas composes `bitbake`, `openembedded-core`, `meta-yocto` and
`meta-openembedded` at the Wrynose 6.0.2 commits. The layer carries its own `COPYING.MIT`, as Yocto
layers do.

## The development container

The base is Ubuntu 26.04 pinned by its multi-arch index digest, with apt pinned to a snapshot of the
Ubuntu archive so a rebuild resolves the same package versions on amd64 and arm64.
CMake is installed from Kitware's prebuilt 4.3.1 tarball, verified by checksum.

| Component  | Wrynose 6.0.2 | Image                        | Delta            |
|------------|---------------|------------------------------|------------------|
| gcc/g++    | 15.3.0        | 15.2.0 (`gcc-15`)            | one patch behind |
| glibc      | 2.43          | 2.43                         | exact            |
| binutils   | 2.46.1        | 2.46                         | one patch behind |
| CMake      | 4.3.1         | 4.3.1 (tarball, SHA256)      | exact            |
| Ninja      | 1.13.2        | 1.13.2                       | exact            |
| LLVM/clang | 22.1.2        | 22.1.2 (`clang-22` packages) | exact            |
| OpenSSL    | 3.5.7         | 3.5.5                        | one patch behind |
| GoogleTest | 1.17.0        | 1.17.0 (CMake config)        | exact            |

The Dockerfile has two stages:
- `ci` holds the toolchain, the baseline libraries and the quality tools. It removes the stock
  `ubuntu` user and runs as root, which is what a GitHub Actions job container expects.
- `dev` adds a non-root `dev` user at a build-arg UID (default 1000), sudo and zsh. Its entrypoint
  remaps `dev` to `HOST_UID`/`HOST_GID` when they are passed at `docker run`, then drops privileges,
  so files under `/work` come out owned by you. A devcontainer gets the same from `remoteUser: dev`
  plus `updateRemoteUserUID`.

The usual `docker run` line mounts the parent of your checkouts, so sibling repositories are
reachable for the local-checkout override described below, and keeps ccache in a named volume:

```sh
docker run --rm -it -e HOST_UID=$(id -u) -e HOST_GID=$(id -g) \
  -v "$PWD":/work -v cxx-cmake-ccache:/home/dev/.ccache \
  ghcr.io/sherlockinspace/cxx-cmake-container/dev:X.Y.Z@sha256:<digest>
```

The images are `ghcr.io/sherlockinspace/cxx-cmake-container/ci`, `…/dev` and `…/yocto`. Tags are
`X.Y.Z` and `X.Y`, plus `X` once the major is non-zero. `X.Y` and `X` are moving tags. If something
must not move, pin `X.Y.Z` or a digest; CI and the devcontainer pin a digest.
- A major bump means the toolchain baseline moved.
- A minor means a tool was added or the Wrynose point release advanced.
- A patch is a rebuild at the same versions.

Each image is a two-architecture manifest built natively per arch and published with an SBOM and
provenance. The base index digest, the apt snapshot ID and the full `dpkg` manifest are recorded in
`/etc/cxx-cmake-container/versions.txt`. The smoke test runs on both stages and both architectures
and checks every row of the table against a checked-in expected-versions file.

## The library's build contract

The contract is what a consumer, a recipe or an SDK shell can rely on after `cmake --install`:

```
<prefix>/include/util/*.hpp               FILE_SET HEADERS, BASE_DIRS inc → #include <util/x.hpp>
<prefix>/lib/libutil.so.X.Y.Z             VERSION = project version
<prefix>/lib/libutil.so.X                 SOVERSION = major (0 until 1.0)
<prefix>/lib/libutil.so                   development symlink
<prefix>/lib/cmake/util/utilConfig.cmake  + utilConfigVersion.cmake + utilTargets*.cmake
```

Paths go through `GNUInstallDirs` and `BUILD_SHARED_LIBS` selects shared or static. The target is
exported as `util::util` and aliased to the same name in-tree, so `add_subdirectory` and installed
consumers write the same link line. The version file uses `SameMinorVersion` before 1.0 and
`SameMajorVersion` after. The config file carries no `find_dependency`; nothing the library links
appears in an installed header. A consumer needs two lines:

```cmake
find_package(util CONFIG REQUIRED)
target_link_libraries(app PRIVATE util::util)
```

The `functional` tier proves this on every pull request: install into a scratch prefix, then
configure, build and run that consumer against it. The recipe packages the same layout (`util-dev`
holds `include/util` and `lib/cmake/util`) and an SDK shell resolves it from
`$OECORE_TARGET_SYSROOT`.

Some things worth knowing about the tree:
- It declares `cmake_minimum_required(VERSION 4.3)`, and C++23 per target with
  `target_compile_features(util PUBLIC cxx_std_23)`.
- `include(CTest)` keeps `BUILD_TESTING` at CMake's default of ON; the recipe turns it OFF unless
  ptest packaging is enabled. Docs and benchmarks are OFF by default.
- `WARNINGS_AS_ERRORS` is ON in the CI presets and OFF in `dev`.
- The unit-test binary is `util_unit_tests`. `gtest_discover_tests` runs with
  `DISCOVERY_MODE PRE_TEST`, so a cross build never executes it on the build host.

`CMakePresets.json` holds the only build commands anyone types: `dev`, `release`, `asan-ubsan`,
`tsan`, `coverage` and `target`, each building into `build/<preset>` so host and target artifacts
never collide in a shared `/work` mount. `target` is only usable inside the yocto image.
No preset names a toolchain file. Source the SDK's `environment-setup-*` script and it exports
`CMAKE_TOOLCHAIN_FILE` and the compiler variables, so `cmake --preset target` cross-compiles the
tree unchanged. BitBake never reads presets; `cmake.bbclass` generates its own toolchain file.

## How dependencies flow

The vendored `cmake/CPM.cmake` resolves like this:

```
CPMAddPackage(NAME OpenSSL VERSION 3.5 ...)
  with CPM_USE_LOCAL_PACKAGES=ON and CPM_LOCAL_PACKAGES_ONLY=ON (the defaults):
  ├─ CPM_OpenSSL_SOURCE set?   → add_subdirectory of that local checkout
  ├─ find_package succeeds     → "CPM: Using local package OpenSSL" in the configure log
  └─ not found                 → configure error; nothing is downloaded
  with -DCPM_LOCAL_PACKAGES_ONLY=OFF:
  └─ not found                 → fetch the declared version (CPM_SOURCE_CACHE shares checkouts)
```

Under BitBake the recipe says `DEPENDS = "openssl"`, the sysroot provides it, and `find_package`
underneath CPM finds it. The local-checkout override is the day-to-day tool: developing the app
against an unreleased library change is `-DCPM_util_SOURCE=/work/cxx-cmake-library` (or the same
name in the environment), which is why the container mounts the parent directory.

## The three developer loops

- The native build in the container, where almost all work happens. Open the repository in the
  devcontainer or run the `docker run` line above, then
  `cmake --preset dev && cmake --build --preset dev && ctest --preset dev`, on x86-64 or arm64.
- The recipe loop, for anyone changing the install contract or the layer. It runs on a workstation
  through `kas-container` against a persistent `SSTATE_DIR` and `DL_DIR` kept outside the build
  tree, with the component commits held in a committed `kas lock` file. `kas build --target util`
  takes the recipe through `do_package_qa`, where install-layout mistakes surface as QA errors.
  `EXTERNALSRC:pn-util = "/work/cxx-cmake-library"` points the recipe at your checkout so you
  iterate without pushing. `MACHINE` is `qemuarm64`, an arm64 tune that needs no BSP layer; the
  build produces packages and executes nothing.
- Cross-building inside the yocto image, described under "The board and the SDK image".

Tests run natively or not at all: in cloud CI on GitHub's x86-64 and arm64 runners inside the `ci`
image, and on the target through `ptest-runner`.

## CI across the repositories

The library and app pipelines have all four tiers. The container has a gate and a release row; the
layer has a gate, a push-triggered BitBake row on the self-hosted runner, and a release row.

| Tier       | When                        | What                                                                                 | Answers                                         |
|------------|-----------------------------|--------------------------------------------------------------------------------------|-------------------------------------------------|
| PR gate    | every pull request          | GCC + clang, Debug + Release; unit + functional; format, tidy, ASan, UBSan; coverage | did this change break anything?                 |
| post-merge | every push to `main`        | the full matrix with the arm64 leg; TSan, CodeQL, the Doxygen gate                   | does everything pass, and which merge broke it? |
| weekly     | a cron, with no new commits | post-merge again; opens an issue on failure                                          | did the environment drift under us?             |
| release    | a release being created     | benchmarks against the previous release                                              | is this shippable?                              |

Every library job runs in the `ci` image pinned by digest, except one plain-distro leg (an
`ubuntu-26.04` hosted runner with apt gcc-15 and `pip install cmake==4.3.1`) that proves the build
has no hidden dependence on the container.

The container's gate builds both stages natively on an x86-64 and an arm64 runner, runs the smoke
test on each and pushes nothing; publishing happens only from the release workflow.

The layer's hosted gate runs `yocto-check-layer` through `kas-container`. It uses a second kas
configuration that leaves this layer out of `BBLAYERS`, since the checker refuses a layer that's
already enabled. The real `kas build` of the recipes runs on a self-hosted runner whose labels come
from the repository variable `BITBAKE_RUNS_ON`, on push and manual dispatch only and never for fork
pull requests. The SDK job runs there too and attaches installers, checksums and the lock file to a
GitHub Release.

## How a release happens

```
PR titled "feat(bloom): expected-based create()"  ──lint──▶  squash-merge
    └─▶ main: "feat(bloom): expected-based create() (#123)"
          └─▶ release-please updates its rolling release PR: CHANGELOG + project(VERSION …)
                └─▶ merge ──▶ tag vX.Y.Z + GitHub Release, both from that same commit
                      ├─ container: build ci and dev per arch, push by digest, merge manifests,
                      │             SBOM + provenance → …/ci:X.Y.Z and …/dev:X.Y.Z
                      ├─ library:   benchmark gate against the previous release
                      └─ layer:     recipe re-pinned to tag=v${PV}, SRCREV = the tag's commit
```

release-please reads the commit subjects on `main`: `fix:` is a patch, `feat:` a minor, `!` or a
`BREAKING CHANGE` footer a major. It rewrites the annotated `project(VERSION …)` line in
`CMakeLists.txt` before the tag is cut from that commit. Publishing runs in the same workflow, gated
on release-please's `release_created` output: a tag pushed with `GITHUB_TOKEN` triggers no workflow
of its own.

The layer pins a release by tag, with `SRCREV` set to the tag's commit and `LIC_FILES_CHKSUM`
computed from the library's own `LICENSE`, so a licence change breaks the recipe. Each library
release is installed into the container image, and that is where the app picks up `util`. The app's
CPM declaration only states the minimum version it needs.

## The board and the SDK image

The board lane runs tests on real hardware. A self-hosted runner on or attached to a board runs
`ptest-runner util` on the target, dispatch-only. For the performance tier it wraps
`ctest -L performance` with a power-capture tool; that tier emits benchmark JSON, so a board result
with a `J` unit flows into the dashboard. Adding a board means adding its `MACHINE` beside
`qemuarm64` in the kas configuration.

The yocto image is the SDK. The layer's SDK job publishes the installers, and they're unpacked into
an image published as `…/yocto` under the shared semver. Its sysroot carries `util-dev`. Inside its
shell `cmake --preset target` cross-compiles either template for the board, and what it builds runs
only there.
