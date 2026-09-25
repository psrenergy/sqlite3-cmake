cmake_minimum_required(VERSION 3.17)

set(SQLITE3_VERSION "3.50.2")
set(SQLITE3_DOWNLOAD_URL "https://sqlite.org/2025/sqlite-amalgamation-3500200.zip")
set(SQLITE3_SHA3_256 "75c118e727ee6a9a3d2c0e7c577500b0c16a848d109027f087b915b671f61f8a")

project(sqlite3 VERSION ${SQLITE3_VERSION} LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel")
if(CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
    set(CMAKE_DEFAULT_BUILD_TYPE "Release")
endif()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(BUILD_SHARED_LIBS)
    set(sqlite3_BUILD_SHARED_LIBS_DEFAULT ON)
else()
    set(sqlite3_BUILD_SHARED_LIBS_DEFAULT OFF)
endif()

option(sqlite3_BUILD_SHARED_LIBS "Build shared libraries" ${sqlite3_BUILD_SHARED_LIBS_DEFAULT})
option(sqlite3_ENABLE_THREADSAFE "Build a thread-safe library" ON)
option(sqlite3_ENABLE_DYNAMIC_EXTENSIONS "Support loadable extensions" ON)
option(sqlite3_ENABLE_MATH "SQL math functions" ON)
option(sqlite3_ENABLE_FTS3 "Include FTS3 support" OFF)
option(sqlite3_ENABLE_FTS4 "Include FTS4 support" ON)
option(sqlite3_ENABLE_FTS5 "Include FTS5 support" ON)
option(sqlite3_ENABLE_RTREE "Include rtree support" ON)
option(sqlite3_ENABLE_SESSION "Enable the session extension" OFF)
option(sqlite3_OMIT_DEPRECATED "Omit deprecated stuff" ON)

if (sqlite3_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

add_library(SQLite3)
add_library(SQLite::SQLite3 ALIAS SQLite3)

include(CMakePackageConfigHelpers)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(FetchContent)
include(GNUInstallDirs)

FetchContent_Declare(sqlite3_ext URL ${SQLITE3_DOWNLOAD_URL} URL_HASH SHA3_256=${SQLITE3_SHA3_256})
FetchContent_MakeAvailable(sqlite3_ext)

target_include_directories(SQLite3
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${sqlite3_ext_SOURCE_DIR}>
    PRIVATE
        ${sqlite3_ext_SOURCE_DIR}
)

function(check_and_define items check_function)
    foreach(item ${${items}})
        string(TOUPPER ${item} item_upper)
        string(MAKE_C_IDENTIFIER ${item_upper} item_upper)
        if(${check_function} STREQUAL "check_include_files")
            check_include_files(${item} ${item_upper})
        elseif(${check_function} STREQUAL "check_type_size")
            check_type_size(${item} ${item_upper})
        endif()
        if(${item_upper})
            target_compile_definitions(SQLite3 PRIVATE "HAVE_${item_upper}=1")
        endif()
    endforeach()
    unset(${items})
endfunction()

set(HEADERS "dlfcn.h" "stdint.h" "inttypes.h" "utime.h")
check_and_define(HEADERS check_include_files)

set(TYPES int8_t uint8_t int16_t uint16_t uint32_t)
check_and_define(TYPES check_type_size)

# check_symbol_exists needs a declaration, so it stays correct when try_compile does not link (iOS toolchains).
# STRERROR_R_CHAR_P is deliberately not probed: sqlite3.c defines _GNU_SOURCE, so glibc selects the GNU variant itself.
check_symbol_exists(fdatasync unistd.h HAVE_FDATASYNC)
check_symbol_exists(usleep unistd.h HAVE_USLEEP)
check_symbol_exists(localtime_r time.h HAVE_LOCALTIME_R)
check_symbol_exists(gmtime_r time.h HAVE_GMTIME_R)
check_symbol_exists(nanosleep time.h HAVE_NANOSLEEP)
check_symbol_exists(strerror_r string.h HAVE_STRERROR_R)
check_symbol_exists(posix_fallocate fcntl.h HAVE_POSIX_FALLOCATE)
foreach(var HAVE_FDATASYNC HAVE_USLEEP HAVE_LOCALTIME_R HAVE_GMTIME_R HAVE_NANOSLEEP HAVE_STRERROR_R HAVE_POSIX_FALLOCATE)
    if(${var})
        target_compile_definitions(SQLite3 PRIVATE ${var}=1)
    endif()
endforeach()

target_sources(SQLite3 PRIVATE ${sqlite3_ext_SOURCE_DIR}/sqlite3.c)

if(WIN32 AND sqlite3_BUILD_SHARED_LIBS)
    target_compile_definitions(SQLite3 PRIVATE "SQLITE_API=__declspec(dllexport)" INTERFACE "SQLITE_API=__declspec(dllimport)")
endif()

# These change what sqlite3.h declares, so consumers need them too
target_compile_definitions(SQLite3 PUBLIC
    $<$<BOOL:${sqlite3_ENABLE_SESSION}>:SQLITE_ENABLE_SESSION SQLITE_ENABLE_PREUPDATE_HOOK>
)

target_compile_definitions(SQLite3 PRIVATE
    $<IF:$<BOOL:${sqlite3_ENABLE_THREADSAFE}>,SQLITE_THREADSAFE=1 _REENTRANT=1,SQLITE_THREADSAFE=0>
    $<$<NOT:$<BOOL:${sqlite3_ENABLE_DYNAMIC_EXTENSIONS}>>:SQLITE_OMIT_LOAD_EXTENSION=1>
    $<$<BOOL:${sqlite3_ENABLE_MATH}>:SQLITE_ENABLE_MATH_FUNCTIONS>
    $<$<AND:$<BOOL:${sqlite3_ENABLE_FTS3}>,$<NOT:$<BOOL:${sqlite3_ENABLE_FTS4}>>>:SQLITE_ENABLE_FTS3>
    $<$<BOOL:${sqlite3_ENABLE_FTS4}>:SQLITE_ENABLE_FTS4>
    $<$<BOOL:${sqlite3_ENABLE_FTS5}>:SQLITE_ENABLE_FTS5>
    $<$<BOOL:${sqlite3_ENABLE_RTREE}>:SQLITE_ENABLE_RTREE SQLITE_ENABLE_GEOPOLY>
    $<$<BOOL:${sqlite3_OMIT_DEPRECATED}>:SQLITE_OMIT_DEPRECATED>
    $<$<CONFIG:Debug>:SQLITE_DEBUG SQLITE_ENABLE_SELECTTRACE SQLITE_ENABLE_WHERETRACE>
)

target_compile_definitions(SQLite3 PRIVATE SQLITE_ENABLE_EXPLAIN_COMMENTS SQLITE_DQS=0 SQLITE_ENABLE_DBPAGE_VTAB SQLITE_ENABLE_STMTVTAB SQLITE_ENABLE_DBSTAT_VTAB)

if(sqlite3_ENABLE_DYNAMIC_EXTENSIONS)
    target_link_libraries(SQLite3 PRIVATE ${CMAKE_DL_LIBS})
endif()

if(sqlite3_ENABLE_MATH OR sqlite3_ENABLE_FTS5)
    check_library_exists(m log "" HAVE_LIBM) # false on MSVC, which has no separate libm
    if(HAVE_LIBM)
        target_link_libraries(SQLite3 PRIVATE m)
    endif()
endif()

if(sqlite3_ENABLE_THREADSAFE)
    find_package(Threads REQUIRED)
    target_link_libraries(SQLite3 PRIVATE Threads::Threads)
endif()

set(SQLITE3_HEADERS ${sqlite3_ext_SOURCE_DIR}/sqlite3.h ${sqlite3_ext_SOURCE_DIR}/sqlite3ext.h)
set_target_properties(SQLite3 PROPERTIES
    VERSION "0.8.6"
    SOVERSION "0"
    PUBLIC_HEADER "${SQLITE3_HEADERS}"
    OUTPUT_NAME ${PROJECT_NAME}
)

install(TARGETS SQLite3
    EXPORT sqlite3-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT sqlite3-targets
    FILE sqlite3-targets.cmake
    NAMESPACE SQLite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sqlite3
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sqlite3-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sqlite3-config-version.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sqlite3
)
