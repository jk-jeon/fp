cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(jkj_fp_benchmark LANGUAGES CXX)

# ---- Add dependencies ----

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
if (NOT TARGET fmt)
    FetchContent_Declare(fmt SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/fmt")
    FetchContent_MakeAvailable(fmt)
endif()

# ---- Fixed-precision Benchmark ----

add_executable(fixed_precision_benchmark
        include/fixed_precision_benchmark.h
        source/fixed_precision_benchmark.cpp
        source/fp_fixed_precision.cpp
        source/ryu_fixed_precision.cpp
        source/fmt_fixed_precision.cpp)

target_compile_features(fixed_precision_benchmark PRIVATE cxx_std_17)

target_include_directories(fixed_precision_benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(fixed_precision_benchmark
        PRIVATE
        ryu::ryu
        fmt::fmt
        jkj_fp::common
        jkj_fp::charconv)

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(fixed_precision_benchmark PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
    target_link_options(fixed_precision_benchmark PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
    set_target_properties(fixed_precision_benchmark PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()