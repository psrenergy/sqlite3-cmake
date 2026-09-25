# sqlite3-cmake

[![Build](https://github.com/psrenergy/sqlite3-cmake/actions/workflows/build.yml/badge.svg)](https://github.com/psrenergy/sqlite3-cmake/actions/workflows/build.yml)

libsqlite3 with CMake support.

Maintained fork of the archived [sjinks/sqlite3-cmake](https://github.com/sjinks/sqlite3-cmake). Tags follow the SQLite version (`vX.Y.Z`) and are created automatically when a SQLite update is merged.

## Usage with [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)

```cmake
include(FetchContent)
FetchContent_Declare(sqlite3 GIT_REPOSITORY https://github.com/psrenergy/sqlite3-cmake GIT_TAG v3.50.2)
FetchContent_MakeAvailable(sqlite3)

target_link_libraries(mytarget SQLite::SQLite3)
```

## Usage with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)

```cmake
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:psrenergy/sqlite3-cmake@3.50.2")
```

## Usage as an installed package

```cmake
find_package(sqlite3 REQUIRED CONFIG) # CONFIG: plain find_package(SQLite3) picks CMake's FindSQLite3 module instead
target_link_libraries(mytarget SQLite::SQLite3)
```

## Options

| Option | Default | Effect |
|---|---|---|
| `sqlite3_BUILD_SHARED_LIBS` | `BUILD_SHARED_LIBS` | Build a shared library |
| `sqlite3_ENABLE_THREADSAFE` | `ON` | `SQLITE_THREADSAFE=1` (`OFF` gives `0`: no mutexes, single-threaded use only) |
| `sqlite3_ENABLE_DYNAMIC_EXTENSIONS` | `ON` | Loadable extensions (`OFF` sets `SQLITE_OMIT_LOAD_EXTENSION`) |
| `sqlite3_ENABLE_MATH` | `ON` | SQL math functions |
| `sqlite3_ENABLE_FTS3` | `OFF` | FTS3 (implied by FTS4) |
| `sqlite3_ENABLE_FTS4` | `ON` | FTS3 and FTS4 |
| `sqlite3_ENABLE_FTS5` | `ON` | FTS5 |
| `sqlite3_ENABLE_RTREE` | `ON` | R*Tree and Geopoly |
| `sqlite3_ENABLE_SESSION` | `OFF` | Session extension and pre-update hook |
| `sqlite3_OMIT_DEPRECATED` | `ON` | `SQLITE_OMIT_DEPRECATED` |

Unlike stock SQLite, the library is always built with `SQLITE_DQS=0` (double-quoted string literals are rejected) and with the `dbpage`, `dbstat` and `stmt` virtual tables.
