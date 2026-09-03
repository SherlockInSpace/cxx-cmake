# Design

How the four cxx-cmake repositories fit together, for someone who wants to use or extend the
template. Why each piece is the way it is, and what we rejected, is in [DECISIONS.md](DECISIONS.md).

## The four repositories

```
cxx-cmake-container ─ ci / dev images ─▶ cxx-cmake-library ─ installed util ─▶ cxx-cmake-app
        │                                        │                                   │
        │ yocto image (from the SDK)             │ util recipe pins a release tag    │ app recipe
        ▼                                        ▼                                   ▼
              meta-cxx-cmake — Yocto layer pinned with kas: recipes, ptest packaging, SDK job
```

`cxx-cmake-library` is the library template: a small C++23 library called `util` (a Bloom filter
hashed through OpenSSL, a vendored thread pool, a header-only `bit` module) that gives the
machinery something real to build. What you keep is the install contract, the test tiers, the CI
layout and the release flow; the code is there to be replaced: start with "Use this template",
rename `util`, delete the rest.

`cxx-cmake-app` is the application template. It consumes the installed `util` as a downstream
project would and adds spdlog as the third-party dependency example, a pytest integration tier
and an optional Tracy build the library never carries; its CI calls the library's workflows.

`cxx-cmake-container` builds the images everything else builds inside: one Dockerfile with two
stages, a smoke test asserting the toolchain contract, and a release workflow publishing to GHCR.

`meta-cxx-cmake` is the Yocto layer: a recipe for `util` and one for the app, a kas configuration
pinned to the exact Wrynose component commits, ptest packaging and the SDK job. It is the proof
that the install contract holds under BitBake. All four are MIT with REUSE metadata (SPDX
headers, `REUSE.toml`, `reuse lint` in CI); the layer also carries `COPYING.MIT`.

## The development container

The container tracks the Yocto baseline, Wrynose 6.0.2, at the version level. The base is Ubuntu
26.04 pinned by its multi-arch index digest, with apt pinned to a snapshot of the Ubuntu archive
so a rebuild resolves identical package versions on amd64 and arm64. CMake comes from a
checksummed Kitware tarball because no Ubuntu archive carries 4.3. Parity is version-level only;
behavioural parity with the Yocto toolchain is the yocto image's job.

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

The Dockerfile has two stages. `ci` holds the toolchain, the baseline libraries and the quality
tools; it sets no `USER`, removes the stock `ubuntu` user and runs as root, which is what a
GitHub Actions job container expects. `dev` builds on `ci` for people: a non-root `dev` user
created at a build-arg UID (default 1000), sudo and zsh. Its entrypoint remaps `dev` to
`HOST_UID` and `HOST_GID` when they are passed at `docker run`, then drops privileges. One
published image therefore serves any host UID, and files under `/work` come out owned by you; a
devcontainer gets the same from `remoteUser: dev` plus `updateRemoteUserUID`. The usual `docker
run` line mounts the parent of your checkouts so sibling repositories are reachable for the
local-checkout override described below, and keeps ccache in a named volume so a fresh container
starts warm:

```sh
docker run --rm -it -e HOST_UID=$(id -u) -e HOST_GID=$(id -g) \
  -v "$PWD":/work -v cxx-cmake-ccache:/home/dev/.ccache \
  ghcr.io/sherlockinspace/cxx-cmake-container/dev:X.Y.Z@sha256:<digest>
```

The images publish to GHCR as `…/cxx-cmake-container/ci`, `…/dev` and `…/yocto`, sharing one
semver. Tags are `X.Y.Z`, `X.Y`, and `X` once the major is non-zero; there is no `latest`. `X.Y`
and `X` move because a workstation pulling a minor is a convenience; anything that must not move
names `X.Y.Z` or, as CI and the devcontainer do, a digest. A major bump means the toolchain
baseline moved, a minor means a tool was added or the Wrynose point release advanced, a patch is
a rebuild at the same versions. Each image is a two-architecture manifest built natively per
arch, published with an SBOM and provenance.

"Reproducible" means two things. If you consume the image, it is the digest: CI and the
devcontainer name `tag@sha256:…`, which resolves to the same bytes forever. If you rebuild the
image, it is the base index digest and the apt snapshot ID, recorded with the full `dpkg`
manifest in `/etc/cxx-cmake-container/versions.txt`, plus the CMake tarball checksum pinned in
the Dockerfile. The smoke test runs on both stages and both architectures and asserts every row
of the table against a checked-in expected-versions file, so a silent version move fails before
it publishes.

## The library's build contract

The contract is what a consumer, a recipe or an SDK shell can rely on after `cmake --install`:

```
<prefix>/include/util/*.hpp               FILE_SET HEADERS, BASE_DIRS inc → #include <util/x.hpp>
<prefix>/lib/libutil.so.X.Y.Z             VERSION = project version
<prefix>/lib/libutil.so.X                 SOVERSION = major (0 until 1.0)
<prefix>/lib/libutil.so                   development symlink
<prefix>/lib/cmake/util/utilConfig.cmake  + utilConfigVersion.cmake + utilTargets*.cmake
```

Paths go through `GNUInstallDirs`. The target is exported as `util::util` and aliased to the same
name in-tree, so `add_subdirectory` and installed consumers write the same link line;
`BUILD_SHARED_LIBS` selects shared or static. The version file uses `SameMinorVersion` before
1.0 and `SameMajorVersion` after. The config file carries no `find_dependency`, because nothing
the library links appears in an installed header. A consumer needs two lines, includes
`<util/bloom.hpp>` and gets a `std::expected` back from `util::Bloom::create(k)` rather than an
exception:

```cmake
find_package(util CONFIG REQUIRED)
target_link_libraries(app PRIVATE util::util)
```

The `functional` tier proves exactly this on every pull request, under CTest: install into a
scratch prefix, configure that consumer against it, build it, run it. The same layout is what the
recipe packages (`util-dev` holds `include/util` and `lib/cmake/util`) and what an SDK shell
resolves from `$OECORE_TARGET_SYSROOT`, so one test covers the host, the package and the cross
case.

The tree declares `cmake_minimum_required(VERSION 4.3)`. C++23 is declared per target with
`target_compile_features(util PUBLIC cxx_std_23)`. First-party functions return `std::expected`
and never throw; the vendored thread pool keeps its future-based interface, so exceptions can
still surface from tasks handed to it. `include(CTest)` keeps `BUILD_TESTING` at CMake's default
of ON; the recipe turns it OFF unless ptest packaging is enabled; docs and benchmarks are OFF by
default. Warnings come from a private `util_warnings` interface target that is never exported;
the CI presets set `WARNINGS_AS_ERRORS=ON` and the `dev` preset does not.

`CMakePresets.json` holds the only build commands anyone types: `dev`, `release`, `asan-ubsan`,
`tsan` and `coverage`, each building into `build/<preset>` so host and target artifacts never
collide in a shared `/work` mount. No preset names a toolchain file, which is what lets a Yocto
SDK cross-compile the tree unchanged: sourcing the SDK's `environment-setup-*` script exports
`CMAKE_TOOLCHAIN_FILE` and the compiler variables, CMake honours them, and `cmake --preset
release` produces target binaries (BitBake itself never reads presets; `cmake.bbclass` generates
its own toolchain file). Tests are one `util_unit_tests` binary registered with
`gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`, which keeps a cross build from executing
the test binary on the build host; tiers are CTest labels.

## How dependencies flow

Every dependency in both templates is declared through the vendored `cmake/CPM.cmake`, pinned
by version and SHA256 and bumped by pull request. The defaults make resolution local and closed:

```
CPMAddPackage(NAME OpenSSL VERSION 3.5 ...)
  with CPM_USE_LOCAL_PACKAGES=ON and CPM_LOCAL_PACKAGES_ONLY=ON (the defaults):
  ├─ CPM_OpenSSL_SOURCE set?   → add_subdirectory of that local checkout
  ├─ find_package succeeds     → "CPM: Using local package OpenSSL" in the configure log
  └─ not found                 → configure error; nothing is downloaded
  with -DCPM_LOCAL_PACKAGES_ONLY=OFF:
  └─ not found                 → fetch the declared version (CPM_SOURCE_CACHE shares checkouts)
```

On the host `find_package` searches the pinned container image; on the target it searches the
Yocto sysroot.

The local-checkout override is the day-to-day tool: developing the app against an unreleased
library change is `-DCPM_util_SOURCE=/work/cxx-cmake-library` (or the same name in the
environment), which is why the container mounts the parent directory. When two changes must land
together, the dependency merges first, a new image or pin is published, and the dependent's pull
request bumps it: two reviewable steps, deterministic because every baseline is versioned. The
same rules make the tree Yocto-safe without special-casing: BitBake's `cmake.bbclass` forces
`FETCHCONTENT_FULLY_DISCONNECTED=ON` and `CMAKE_FIND_ROOT_PATH_MODE_* ONLY`, the posture the
templates already run in; the recipe says `DEPENDS = "openssl"`, the sysroot provides it, and
`find_package` underneath CPM finds it.

## The three developer loops

The first loop is the native build in the container, where almost all work happens: open the
repository in the devcontainer or run the `docker run` line above, then `cmake --preset dev &&
cmake --build --preset dev && ctest --preset dev`, natively on x86-64 or arm64.

The second loop is the recipe loop, for anyone changing the install contract or the layer. It
runs on a workstation through the `kas-container` wrapper, which runs BitBake in kas's own image
against a persistent `SSTATE_DIR` and `DL_DIR` kept outside the build tree, with the component
commits held in a committed `kas lock` file. `kas build --target util` takes the recipe through
`do_package_qa`, where install-layout mistakes surface as QA errors: an unversioned `.so` in the
runtime package, a CMake config file outside `-dev`. An `externalsrc` override
(`EXTERNALSRC:pn-util = "/work/cxx-cmake-library"`) points the recipe at your checkout so you
iterate without pushing. `MACHINE` is `qemuarm64`, an arm64 tune that needs no BSP layer; the
build produces packages and nothing executes them.

The third loop, cross-building inside the yocto image, is described under "The board and the SDK
image". Whichever loop produced the binaries, tests run natively or not at all: in cloud CI on
GitHub's x86-64 and arm64 hosted runners, both real machines, inside the `ci` image, and on the
target through `ptest-runner` once a board is attached to CI.

## CI across the repositories

The library and app pipelines have all four tiers. The container has a gate and a release row;
the layer has a gate, a push-triggered BitBake row on the self-hosted runner, and a release row:

| Tier       | Runs on                     | Answers                                         |
|------------|-----------------------------|-------------------------------------------------|
| PR gate    | every pull request          | did this change break anything?                 |
| post-merge | every push to `main`        | does everything pass, and which merge broke it? |
| weekly     | a cron, with no new commits | did the environment drift under us?             |
| release    | a release being created     | is this shippable?                              |

Every library job runs in the `ci` image pinned by digest except one plain-distro leg (an
`ubuntu-26.04` hosted runner with apt gcc-15 and `pip install cmake==4.3.1`) proving the build
has no hidden dependence on the container. Post-merge publishes the Doxygen site and the coverage
HTML to GitHub Pages from one artifact.

The container's gate builds both stages natively on an amd64 and an arm64 runner, runs the smoke
test on each, and pushes nothing; publishing happens only from the release workflow. The layer's
hosted gate runs `yocto-check-layer` through `kas-container` against a second kas configuration
that leaves this layer out of `BBLAYERS`, because the checker refuses a layer already enabled.
The real `kas build` of the recipes runs on a self-hosted runner whose labels come from one
repository variable, `BITBAKE_RUNS_ON`, on push and manual dispatch only and never for fork pull
requests; moving it to a rented machine is registering that machine under the same labels. The
SDK job runs there too and attaches installers, checksums and the lock file to a GitHub Release.

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
`BREAKING CHANGE` footer a major. Its generic updater rewrites the annotated `project(VERSION …)`
line in `CMakeLists.txt` before the tag is cut from that commit. Publishing runs inside the same
workflow, since a tag pushed with the workflow token triggers nothing on its own. The layer pins
a release by tag, with `SRCREV` set to the tag's commit and `LIC_FILES_CHKSUM` computed from the
library's own `LICENSE`, so a licence change breaks the recipe by design. The app gets a released
`util` from the container image, into which each release is installed; its CPM declaration
states only the minimum it needs.

## The board and the SDK image

Two pieces are designed around seams that already exist rather than built: the lane that runs
tests on hardware, and the image that cross-compiles for it.

The board lane is test execution on real hardware: a self-hosted runner on or attached to a
board, dispatch-only, running `ptest-runner util` on the target and, for the performance tier,
wrapping `ctest -L performance` with a power-capture tool. What it needs is in place: the recipe
packages ptest, the performance tier is label-selected and emits benchmark JSON so a board result
with a `J` unit flows into the existing dashboard, and adding the board means adding its
`MACHINE` beside `qemuarm64` in the kas configuration.

The SDK image is the yocto flavour of the container: the installers the layer's SDK job
publishes, unpacked into an image under the shared semver as `…/yocto`, whose sysroot carries
`util-dev`. Inside its shell `cmake --preset target` cross-compiles either template for the
board, and what it builds executes only there, through the lane above. Nothing on the library
side changes for it: no preset names a toolchain file, and `target` is the one preset name left
undefined until the SDK image exists.
