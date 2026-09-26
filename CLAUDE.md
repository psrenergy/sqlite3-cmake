# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A CMake wrapper around the SQLite amalgamation. It is a maintained fork of the archived `sjinks/sqlite3-cmake`. The repo contains no SQLite sources. At configure time, `CMakeLists.txt` downloads the amalgamation zip with `FetchContent`, verifies it with SHA3-256, and builds one target, `SQLite3`, with the alias `SQLite::SQLite3`. It also installs a CMake config package named `sqlite3`, so consumers use `find_package(sqlite3 CONFIG)`. The main consumer is `psrenergy/quiver`. It pulls this repo via FetchContent at a `vX.Y.Z` tag and builds it on Linux, Windows and macOS, and also cross-compiles it through Dart native-assets hooks (iOS/Android/Linux toolchain files).

## Commands

The smoke test in `test/` is the only test. It is a separate CMake project that consumes the *installed* package, so it covers linking, `find_package` and exported link dependencies. CI (`build.yml`) runs exactly this sequence on ubuntu/windows/macos × {`-DBUILD_SHARED_LIBS=OFF`, `-DBUILD_SHARED_LIBS=ON -Dsqlite3_ENABLE_THREADSAFE=OFF`}:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix "$PWD/install"
cmake -S test -B build-test -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build-test --config Release
PATH="$PWD/install/bin:$PATH" ctest --test-dir build-test -C Release --output-on-failure -V   # PATH is for the Windows DLL
```

The downloaded amalgamation ends up in `build/_deps/sqlite3_ext-src/`. Grep `sqlite3.c` there to check whether a macro is actually read before adding or keeping a compile definition. Several `HAVE_*` defines the build sets are dead.

## Version pinning and releases

- The three `set(SQLITE3_VERSION|SQLITE3_DOWNLOAD_URL|SQLITE3_SHA3_256 "...")` lines at the top of `CMakeLists.txt` are rewritten by `sed` in `.github/workflows/check-for-updates.yml`. Keep their exact format. `SQLITE3_VERSION` must stay three components, because the bot compares it byte-for-byte with sqlite.org's version.
- The bot runs daily. It also rewrites the `GIT_TAG vX.Y.Z` and `sqlite3-cmake@X.Y.Z` strings in `README.md`, but not the CPM.cmake version.
- The bot opens or updates one PR on the fixed branch `update-sqlite`, using `GITHUB_TOKEN`. PRs created with that token do not trigger `pull_request` workflows, so the bot starts `build.yml` itself via `workflow_dispatch`. Keep `workflow_dispatch` in `build.yml`.
- When `CMakeLists.txt` changes on master, `tag.yml` creates the GitHub release and tag `v<SQLITE3_VERSION>`. It skips the release if one already exists.
- A fix to this repo that doesn't change the SQLite version is tagged by hand as `vX.Y.Z.N`.
- Dependabot handles GitHub Actions updates. Actions are pinned by commit SHA with a `# vX.Y.Z` comment.
- This repo is a fork, so `gh pr create` defaults to the archived parent. Always pass `--repo psrenergy/sqlite3-cmake --base master`.

## CMake constraints (each one fixes a real bug; don't regress them)

- **No `try_run`/`check_c_source_runs`.** Consumers cross-compile, and `try_run` hard-errors there. Also never define `STRERROR_R_CHAR_P`. sqlite3.c defines `_GNU_SOURCE`, so it picks the right `strerror_r` variant on glibc, XSI and Android by itself.
- **Use `check_symbol_exists(<fn> <header> HAVE_<FN>)`, not `check_function_exists`.** Toolchains with `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY` (iOS) never link, so `check_function_exists` reports every function as present. That breaks the Darwin build via `posix_fallocate`.
- **Configuration must succeed on MSVC**, which has no libm or libdl. Link `${CMAKE_DL_LIBS}` and a non-fatal `check_library_exists(m ...)` result, and never `FATAL_ERROR` on them. Option variables all carry the `sqlite3_` prefix; an unprefixed `if(ENABLE_*)` silently never runs.
- **Link dependencies are `PRIVATE`.** For static builds they are exported as `$<LINK_ONLY:...>`. If you add a dependency that is an imported package target, such as `Threads::Threads`, add a matching `find_dependency()` to `sqlite3-config.cmake`.
- **Compile definitions that change what `sqlite3.h` declares must be `PUBLIC`**, for example `SQLITE_ENABLE_SESSION` and `SQLITE_ENABLE_PREUPDATE_HOOK`. Everything else stays `PRIVATE`.
- **Windows shared builds need `SQLITE_API`** set to `__declspec(dllexport)` (PRIVATE) and `__declspec(dllimport)` (INTERFACE). Without it the DLL exports nothing and no import `.lib` is produced.
- **Defaults intentionally differ from stock SQLite and are documented in the README options table:**
  - Always on: `SQLITE_DQS=0`, `SQLITE_OMIT_DEPRECATED`, and the dbpage/dbstat/stmt vtabs.
  - `sqlite3_ENABLE_THREADSAFE` defaults to `ON`. Upstream defaulted it to `OFF`.
