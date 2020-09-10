cmake_minimum_required(VERSION 3.12)
project(MoreConcurrentThinking CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
enable_testing()

# Location for any external projects source and installation.
include(ExternalProject)
set(EXTERNAL_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)
set(EXTERNAL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external-install)

# boost.ut unit testing library.
set(BOOST_UT_INSTALL_DIR ${EXTERNAL_INSTALL_DIR}/boost.ut)
ExternalProject_Add(project_boost.ut
    PREFIX ${EXTERNAL_SRC_DIR}/boost.ut
    INSTALL_DIR ${BOOST_UT_INSTALL_DIR}
    GIT_REPOSITORY https://github.com/boost-ext/ut.git
    GIT_TAG ebeb3ca474ee4813a7980b1b06a5da80c876e404
    GIT_SHALLOW TRUE
    CMAKE_ARGS -D CMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               -D BOOST_UT_BUILD_BENCHMARKS=OFF
               -D BOOST_UT_BUILD_EXAMPLES=OFF
               -D BOOST_UT_BUILD_TESTS=OFF
               -D BOOST_UT_ENABLE_RUN_AFTER_BUILD=OFF
)
add_library(boost.ut INTERFACE)
target_include_directories(boost.ut INTERFACE ${BOOST_UT_INSTALL_DIR}/include)

add_subdirectory(class_files)
add_subdirectory(day1)
add_subdirectory(day2)
