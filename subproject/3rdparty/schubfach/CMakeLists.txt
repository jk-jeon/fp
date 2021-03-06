cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(schubfach LANGUAGES CXX)

add_library(schubfach STATIC
        schubfach_32.cc schubfach_32.h schubfach_64.cc schubfach_64.h)
add_library(schubfach::schubfach ALIAS schubfach)

target_compile_features(schubfach PUBLIC cxx_std_17)

target_include_directories(schubfach
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    target_compile_options(schubfach PUBLIC
              /Zi /permissive-
              $<$<CONFIG:Release>:/GL>)
endif()
set_target_properties(schubfach PROPERTIES FOLDER fp/extern)