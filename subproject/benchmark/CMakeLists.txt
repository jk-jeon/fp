cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(benchmark LANGUAGES CXX)

include(FetchContent)
if (NOT TARGET jkj_fp)
    FetchContent_Declare(jkj_fp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
    FetchContent_MakeAvailable(jkj_fp)
endif()
if (NOT TARGET common)
    FetchContent_Declare(common SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../common")
    FetchContent_MakeAvailable(common)
endif()
if (NOT TARGET ryu)
    FetchContent_Declare(ryu SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/ryu")
    FetchContent_MakeAvailable(ryu)
endif()

#set(benchmark_headers include/benchmark.h)

set(benchmark_sources
        source/ryu_printf.cpp)

add_executable(benchmark ${benchmark_headers} ${benchmark_sources})

target_compile_features(benchmark PRIVATE cxx_std_17)

target_include_directories(benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(benchmark
        PRIVATE
        ryu::ryu
        jkj_fp::common
        jkj_fp::charconv)

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(benchmark PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
    target_link_options(benchmark PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
    set_target_properties(benchmark PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()