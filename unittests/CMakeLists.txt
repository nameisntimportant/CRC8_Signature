cmake_minimum_required(VERSION 3.5)

set(TARGET unittests)

enable_testing()

find_package(Boost 1.71 COMPONENTS thread program_options unit_test_framework
             filesystem system REQUIRED)

set(UNDER_TEST_SRCS
    ${SRC_DIRECTORY}/programmoptions.cpp
    ${SRC_DIRECTORY}/datafile.cpp
    ${SRC_DIRECTORY}/dataframe.cpp
    ${SRC_DIRECTORY}/zerofilledmemory.cpp
    ${SRC_DIRECTORY}/concurentmemorypool.cpp
    ${SRC_DIRECTORY}/crchasher.cpp
    ${SRC_DIRECTORY}/datafilewrapper.cpp
    ${SRC_DIRECTORY}/crcsignatureoffile.cpp)

set(UNDER_TEST_HDRS
    ${SRC_DIRECTORY}/programmoptions.h
    ${SRC_DIRECTORY}/datafile.h
    ${SRC_DIRECTORY}/dataframe.h
    ${SRC_DIRECTORY}/zerofilledmemory.h
    ${SRC_DIRECTORY}/concurentmemorypool.h
    ${SRC_DIRECTORY}/datafilewrapper.h
    ${SRC_DIRECTORY}/crchasher.h
    ${SRC_DIRECTORY}/crcsignatureoffile.h
    ${SRC_DIRECTORY}/memorysizeliterals.h
    ${SRC_DIRECTORY}/defs.h
    ${SRC_DIRECTORY}/utils.h)

set(TESTS_SRCS
    main.cpp
    programmoptionstestsuite.cpp
    datafiletestsuite.cpp
    dataframetestsuite.cpp
    datafilewrappertestsuite.cpp
    crchashertestsuite.cpp
    crcsignatureoffiletestsuite.cpp
    testtools.cpp)

set(TESTS_HDRS
    testtools.h
    testdefs.h)

set(UNIT_TESTS_DATA_DIRECTORY ${CMAKE_SOURCE_DIR}/unittestsdata)

add_executable(${TARGET} ${UNDER_TEST_SRCS} ${UNDER_TEST_HDRS} ${TESTS_SRCS} ${TESTS_HDRS})
target_link_libraries(${TARGET} ${Boost_LIBRARIES})
target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIR} ${SRC_DIRECTORY})

add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                   ${UNIT_TESTS_DATA_DIRECTORY}/smallRandFile $<TARGET_FILE_DIR:${TARGET}>)
