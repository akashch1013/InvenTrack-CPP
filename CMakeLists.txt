cmake_minimum_required(VERSION 3.16)
project(StoreForge VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor)
endif()

# Build options
option(STOREFORGE_BUILD_TESTS "Build GoogleTest test suite" ON)
option(STOREFORGE_ENABLE_POSTGRES "Enable PostgreSQL libpqxx backend" ON)
option(STOREFORGE_ENABLE_COVERAGE "Enable code coverage flags" OFF)

if(STOREFORGE_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# Threading support
find_package(Threads REQUIRED)

# PostgreSQL & libpqxx support
set(LIBPQXX_FOUND_OR_OPTIONAL FALSE)
if(STOREFORGE_ENABLE_POSTGRES)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PQXX libpqxx)
    endif()
    if(NOT PQXX_FOUND)
        find_package(pqxx QUIET CONFIG)
    endif()
    if(PQXX_FOUND OR pqxx_FOUND)
        message(STATUS "PostgreSQL libpqxx found - database persistence enabled")
        set(LIBPQXX_FOUND_OR_OPTIONAL TRUE)
    else()
        message(WARNING "PostgreSQL libpqxx not found on host. PostgreSQL backend will compile with conditional headers or fallback stubs.")
    endif()
endif()

# Project include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# StoreForge Core Library Sources
file(GLOB_RECURSE STOREFORGE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/domain/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/repo/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/db/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cli/*.cpp"
)

file(GLOB_RECURSE STOREFORGE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/storeforge/*.hpp"
)

add_library(storeforge_lib STATIC ${STOREFORGE_SOURCES} ${STOREFORGE_HEADERS})

target_include_directories(storeforge_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(storeforge_lib PUBLIC Threads::Threads)

if(STOREFORGE_ENABLE_POSTGRES AND LIBPQXX_FOUND_OR_OPTIONAL)
    target_compile_definitions(storeforge_lib PUBLIC STOREFORGE_HAS_LIBPQXX=1)
    if(TARGET pqxx::pqxx)
        target_link_libraries(storeforge_lib PUBLIC pqxx::pqxx)
    elseif(PQXX_LIBRARIES)
        target_include_directories(storeforge_lib PUBLIC ${PQXX_INCLUDE_DIRS})
        target_link_libraries(storeforge_lib PUBLIC ${PQXX_LIBRARIES})
    endif()
endif()

# Executable: StoreForge CLI
add_executable(storeforge_cli src/main.cpp)
target_link_libraries(storeforge_cli PRIVATE storeforge_lib)

# Testing
if(STOREFORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
