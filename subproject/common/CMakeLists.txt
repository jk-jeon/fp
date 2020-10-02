cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(jkj_fp_common LANGUAGES CXX)

include(FetchContent)
if (NOT TARGET jkj_fp)
    FetchContent_Declare(jkj_fp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
    FetchContent_MakeAvailable(jkj_fp)
endif()

add_library(jkj_fp_common INTERFACE)
add_library(jkj_fp::common ALIAS jkj_fp_common)

target_include_directories(jkj_fp_common
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_compile_features(jkj_fp_common INTERFACE cxx_std_17)

target_link_libraries(jkj_fp_common INTERFACE jkj_fp::fp)

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    target_compile_options(jkj_fp_common INTERFACE
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>)
endif()