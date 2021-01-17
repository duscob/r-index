########################################################################
# CMake build script for R-Index Library.

option(rindex_build_benchmarks "Build all of rindex's benchmarks." ON)

option(rindex_install "Enable installation of r-index. (Projects embedding r-index may want to turn this OFF.)" ON)


cmake_minimum_required(VERSION 2.6)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.. )

project (s-rlbwt)

# Location of internal cmake modules
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${PROJECT_SOURCE_DIR}/cmake)

# Guard against in-source builds and bad build-type strings
include(ConfigSafeGuards)


include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/internal)
include_directories(~/include) #SDSL headers are here

LINK_DIRECTORIES(~/lib) #SDSL lib are here

message("Building in ${CMAKE_BUILD_TYPE} mode")

set(CMAKE_CXX_FLAGS "--std=c++14")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -fstrict-aliasing -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -ggdb -Ofast -fstrict-aliasing -march=native")

#Library setup
set(HEADER_FILES
        ${PROJECT_SOURCE_DIR}/internal/definitions.hpp
        ${PROJECT_SOURCE_DIR}/internal/huff_string.hpp
        ${PROJECT_SOURCE_DIR}/internal/r_index.hpp
        ${PROJECT_SOURCE_DIR}/internal/rle_string.hpp
        ${PROJECT_SOURCE_DIR}/internal/sparse_hyb_vector.hpp
        ${PROJECT_SOURCE_DIR}/internal/sparse_sd_vector.hpp
        ${PROJECT_SOURCE_DIR}/internal/succinct_bit_vector.hpp
        ${PROJECT_SOURCE_DIR}/internal/utils.hpp)

add_library(rindex INTERFACE)
target_sources(rindex INTERFACE ${HEADER_FILES})
target_include_directories(rindex INTERFACE ${PROJECT_SOURCE_DIR}/internal)
#set_target_properties(rindex PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")

add_executable(ri-build ri-build.cpp)
TARGET_LINK_LIBRARIES(ri-build sdsl)
TARGET_LINK_LIBRARIES(ri-build divsufsort)
TARGET_LINK_LIBRARIES(ri-build divsufsort64)

add_executable(ri-locate ri-locate.cpp)
TARGET_LINK_LIBRARIES(ri-locate rindex)
TARGET_LINK_LIBRARIES(ri-locate sdsl)
TARGET_LINK_LIBRARIES(ri-locate divsufsort)
TARGET_LINK_LIBRARIES(ri-locate divsufsort64)

add_executable(ri-count ri-count.cpp)
TARGET_LINK_LIBRARIES(ri-count sdsl)
TARGET_LINK_LIBRARIES(ri-count divsufsort)
TARGET_LINK_LIBRARIES(ri-count divsufsort64)

#add_executable(ri-space ri-space.cpp)
#TARGET_LINK_LIBRARIES(ri-space sdsl)
#TARGET_LINK_LIBRARIES(ri-space divsufsort)
#TARGET_LINK_LIBRARIES(ri-space divsufsort64)

include(internal_utils)

if (rindex_build_benchmarks)
    include(ConfigGBenchmark)

    find_library(GFLAGS_LIB gflags REQUIRED)
    find_package(Threads)

    find_library(SDSL_LIB sdsl REQUIRED)
    find_library(DIVSUFSORT_LIB divsufsort REQUIRED)
    find_library(DIVSUFSORT64_LIB divsufsort64 REQUIRED)
    set(SDSL_LIBS ${SDSL_LIB} ${DIVSUFSORT_LIB} ${DIVSUFSORT64_LIB})
    set(LIBS ${SDSL_LIBS})

    #    cxx_executable_with_flags(query_doc_freq_index_bm "" "${GFLAGS_LIB};benchmark;dret;${LIBS};${GRAMMAR_LIB};${CMAKE_THREAD_LIBS_INIT}" benchmark/query_doc_freq_index_bm.cpp)
    cxx_executable_with_flags(bm_locate "" "${GFLAGS_LIB};benchmark;${LIBS};" benchmark/bm_locate.cpp)
endif ()

########################################################################
#
# Install rules
if (rindex_install)
    if (CMAKE_VERSION VERSION_LESS 2.8.5)
        set(CMAKE_INSTALL_BINDIR "bin" CACHE STRING "User executables (bin)")
        set(CMAKE_INSTALL_LIBDIR "lib${LIB_SUFFIX}" CACHE STRING "Object code libraries (lib)")
        set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE STRING "C header files (include)")
        mark_as_advanced(CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)
    else()
        include(GNUInstallDirs)
    endif()

    install(TARGETS rindex
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rindex")
endif ()
