set(TARGET ${PROJECT_NAME})

find_package(Boost 1.71 COMPONENTS thread program_options REQUIRED)

set(PROJECT_SRCS
    main.cpp
    datafile.cpp
    dataframe.cpp
    zerofilledmemory.cpp
    datafilewrapper.cpp
    crchasher.cpp
    concurentmemorypool.cpp
    crcsignatureoffile.cpp
    programmoptions.cpp)

set(PROJECT_HDRS
    datafile.h
    dataframe.h
    zerofilledmemory.h
    datafilewrapper.h
    crchasher.h
    concurentqueue.h
    concurentmemorypool.h
    utils.h
    crcsignatureoffile.h
    programmoptions.h
    memorysizeliterals.h
    defs.h)

add_executable(${TARGET} ${PROJECT_SRCS} ${PROJECT_HDRS})

target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${TARGET} LINK_PRIVATE ${Boost_LIBRARIES} )
