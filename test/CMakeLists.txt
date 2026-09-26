cmake_minimum_required(VERSION 3.17)
project(sqlite3-smoke LANGUAGES C)

find_package(sqlite3 REQUIRED CONFIG)

add_executable(smoke main.c)
target_link_libraries(smoke PRIVATE SQLite::SQLite3)

enable_testing()
add_test(NAME smoke COMMAND smoke)
