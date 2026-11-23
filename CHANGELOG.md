# Changelog

## [0.4.0](https://github.com/SherlockInSpace/cxx-cmake-library/compare/v0.3.0...v0.4.0) (2026-11-23)


### Features

* **ci:** switches on the reusable workflows for projects without every leg ([7ea6854](https://github.com/SherlockInSpace/cxx-cmake-library/commit/7ea6854f006d2794ea04331ce4928cd27f0530d6)), closes [#138](https://github.com/SherlockInSpace/cxx-cmake-library/issues/138)

## [0.3.0](https://github.com/SherlockInSpace/cxx-cmake-library/compare/v0.2.0...v0.3.0) (2026-11-23)


### Features

* **perf:** Tracy harness targets in the performance tier ([8dec138](https://github.com/SherlockInSpace/cxx-cmake-library/commit/8dec138e0fe50ac457bebd8f5eafdd7f623d7031)), closes [#57](https://github.com/SherlockInSpace/cxx-cmake-library/issues/57)


### Bug Fixes

* **ci:** deploy Pages from a separate job after the upload ([9714f40](https://github.com/SherlockInSpace/cxx-cmake-library/commit/9714f4025161b327fd11e830ccda6e076d79611a)), closes [#127](https://github.com/SherlockInSpace/cxx-cmake-library/issues/127)
* **ci:** switch back explicitly after starting gh-pages ([d14ee15](https://github.com/SherlockInSpace/cxx-cmake-library/commit/d14ee15d627409ff16467f784931040550bcc639)), closes [#131](https://github.com/SherlockInSpace/cxx-cmake-library/issues/131)

## [0.2.0](https://github.com/SherlockInSpace/cxx-cmake-library/compare/v0.1.0...v0.2.0) (2026-11-23)


### Features

* **bench:** bench/ with Google Benchmark behind BUILD_BENCH ([d48eb19](https://github.com/SherlockInSpace/cxx-cmake-library/commit/d48eb191b58d1fd57c9a29df9e9be27450c1fa89)), closes [#54](https://github.com/SherlockInSpace/cxx-cmake-library/issues/54)
* **bench:** thread-pool throughput and expected-vs-throw benchmarks ([f2356e3](https://github.com/SherlockInSpace/cxx-cmake-library/commit/f2356e31241a549a1624834c025ae6088ff2ea95)), closes [#55](https://github.com/SherlockInSpace/cxx-cmake-library/issues/55)
* **bloom:** expected-based create() factory replacing the throwing constructor ([0ec0391](https://github.com/SherlockInSpace/cxx-cmake-library/commit/0ec0391d7ecb99ee6a4263737e069fd42c122af3)), closes [#19](https://github.com/SherlockInSpace/cxx-cmake-library/issues/19)


### Bug Fixes

* **bloom:** migrate MD5 to EVP SHA-256 and drop OpenSSL from the public header ([827c2db](https://github.com/SherlockInSpace/cxx-cmake-library/commit/827c2db0d76e7182b1a069f42393756e6700421d)), closes [#18](https://github.com/SherlockInSpace/cxx-cmake-library/issues/18)
* **bloom:** remove aliasing UB, the shared mutable buffer, and the init-order mismatch ([eb8f32d](https://github.com/SherlockInSpace/cxx-cmake-library/commit/eb8f32de65e363a2f1c3c0177e35b7876575250a)), closes [#17](https://github.com/SherlockInSpace/cxx-cmake-library/issues/17)


### Build

* **cmake:** BUILD_TESTING replaces BUILD_TEST; drop dead test branches and the vendored coverage module ([edd9666](https://github.com/SherlockInSpace/cxx-cmake-library/commit/edd96667bde32bff54058531be6b212af2064fec)), closes [#11](https://github.com/SherlockInSpace/cxx-cmake-library/issues/11)
* **cmake:** CMakePresets.json (dev, release) ([0ea4c59](https://github.com/SherlockInSpace/cxx-cmake-library/commit/0ea4c59ad0b45e5e1ac66d5021273300ee5212c0)), closes [#27](https://github.com/SherlockInSpace/cxx-cmake-library/issues/27)
* **cmake:** declare OpenSSL and GoogleTest through CPM with local-only defaults ([85892ce](https://github.com/SherlockInSpace/cxx-cmake-library/commit/85892ce13bbaeff3dd5eef9eca04a84b4b96a8dd)), closes [#24](https://github.com/SherlockInSpace/cxx-cmake-library/issues/24)
* **cmake:** export to lib/cmake/util with util::util alias, VERSION/SOVERSION and BUILD_SHARED_LIBS ([30efb53](https://github.com/SherlockInSpace/cxx-cmake-library/commit/30efb53480579e8e70b041d053df19019cfe50c3)), closes [#22](https://github.com/SherlockInSpace/cxx-cmake-library/issues/22)
* **cmake:** install headers under include/util via FILE_SET ([b54917d](https://github.com/SherlockInSpace/cxx-cmake-library/commit/b54917debc42ecc46fb47dda8df6046b5a49a9ac)), closes [#21](https://github.com/SherlockInSpace/cxx-cmake-library/issues/21)
* **cmake:** require CMake 4.3 and C++23 ([e103505](https://github.com/SherlockInSpace/cxx-cmake-library/commit/e103505b3a907444520deea6360fd8030ef6cede)), closes [#9](https://github.com/SherlockInSpace/cxx-cmake-library/issues/9)
* **cmake:** require OpenSSL and link Crypto privately ([e426be8](https://github.com/SherlockInSpace/cxx-cmake-library/commit/e426be81c9f66bf6d42aa981f7e5972cfa8846f4)), closes [#15](https://github.com/SherlockInSpace/cxx-cmake-library/issues/15)
* **cmake:** sanitizer presets ([33ff473](https://github.com/SherlockInSpace/cxx-cmake-library/commit/33ff4730b5828d83af0e3c63bd938e6138b44a1d)), closes [#35](https://github.com/SherlockInSpace/cxx-cmake-library/issues/35)
* **cmake:** skip C++ module dependency scanning ([0df6f0a](https://github.com/SherlockInSpace/cxx-cmake-library/commit/0df6f0a3340a27384fd39072964cb6c91a562bc4)), closes [#43](https://github.com/SherlockInSpace/cxx-cmake-library/issues/43)
* **cmake:** warnings INTERFACE target, fixes, and -Werror in CI presets ([6be4e54](https://github.com/SherlockInSpace/cxx-cmake-library/commit/6be4e54a730c7482c872c38fa4acb62791a58b5c)), closes [#41](https://github.com/SherlockInSpace/cxx-cmake-library/issues/41)
* **coverage:** coverage preset with an aggregate gcovr run ([b7f301d](https://github.com/SherlockInSpace/cxx-cmake-library/commit/b7f301dcf4e07ae610f364884157ae2afaaaab59)), closes [#37](https://github.com/SherlockInSpace/cxx-cmake-library/issues/37)
