find_package(Boost 1.71 COMPONENTS program_options thread filesystem system REQUIRED)

set(TARGET integrationtest)

add_executable(${TARGET}
    ${SRC_DIRECTORY}/programmoptions.h ${SRC_DIRECTORY}/programmoptions.cpp
    ${SRC_DIRECTORY}/crchasher.h ${SRC_DIRECTORY}/crchasher.cpp
    ${SRC_DIRECTORY}/dataframe.h ${SRC_DIRECTORY}/dataframe.cpp
    ${SRC_DIRECTORY}/zerofilledmemory.h ${SRC_DIRECTORY}/zerofilledmemory.cpp
    ${SRC_DIRECTORY}/concurentmemorypool.h ${SRC_DIRECTORY}/concurentmemorypool.cpp
    profiler.h integrationtest.cpp)
target_link_libraries(${TARGET} ${Boost_LIBRARIES})
target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIR} ${SRC_DIRECTORY})
