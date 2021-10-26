set(CMAKE_MIN_VERSION 3.5)
cmake_minimum_required(VERSION ${CMAKE_MIN_VERSION})

project(CRC_8_signature LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message ("building was tested with GCC only")
endif()

option(ENABLE_RUNTIME_CHECKS "Compile with sanitizers and debugging flags" OFF)
option(BUILD_UNIT_TESTS "Compile unit tests" OFF)
option(BUILD_INTEGRATION_TEST "Compile integration test" OFF)

add_compile_options(
    -Wall -Wextra -O3

    -pedantic -std=c++17

    -Wsign-conversion -Wsign-promo -Wfloat-equal -Wenum-compare
    -Wshadow -Wformat=2 -Wconversion -Wshift-overflow=2 -Wduplicated-cond -Wlogical-op
    -Wcast-qual -Wcast-align -Wold-style-cast -Wredundant-decls
    -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor
)

if(ENABLE_RUNTIME_CHECKS)
    add_compile_options(
        -D_FORTIFY_SOURCE=2
        -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
        -fstack-protector
    )
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

set(SRC_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
set(ITEGRATION_TEST_DIRECTORY ${CMAKE_SOURCE_DIR}/integrationtest)
set(UNIT_TESTS_DIRECTORY ${CMAKE_SOURCE_DIR}/unittests)

add_subdirectory(${SRC_DIRECTORY})

if (${BUILD_UNIT_TESTS})
    add_subdirectory(${UNIT_TESTS_DIRECTORY})
endif()

if (${BUILD_INTEGRATION_TEST})
    add_subdirectory(${ITEGRATION_TEST_DIRECTORY})
endif()
