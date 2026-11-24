# Tests CMake configuration
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Look for system GTest first, fallback to FetchContent
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    FetchContent_MakeAvailable(googletest)
    add_library(GTest::gtest ALIAS gtest)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

if(TEST_SOURCES)
    add_executable(storeforge_tests ${TEST_SOURCES})
    target_link_libraries(storeforge_tests PRIVATE storeforge_lib GTest::gtest GTest::gtest_main Threads::Threads)
    include(GoogleTest)
    gtest_discover_tests(storeforge_tests)
endif()
