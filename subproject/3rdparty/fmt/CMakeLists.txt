cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(fmt_minimal LANGUAGES CXX)

add_library(fmt INTERFACE)
add_library(fmt::fmt ALIAS fmt)

target_compile_features(fmt INTERFACE cxx_std_17)

target_include_directories(fmt
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    target_compile_options(fmt INTERFACE
              /Zi $<$<CONFIG:Release>:/GL>)
endif()