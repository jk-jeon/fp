cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(jkj_fp_benchmark LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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
if (NOT TARGET grisu_exact)
    FetchContent_Declare(grisu_exact SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/grisu_exact")
    FetchContent_MakeAvailable(grisu_exact)
endif()
if (NOT TARGET schubfach)
    FetchContent_Declare(schubfach SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/schubfach")
    FetchContent_MakeAvailable(schubfach)
endif()
if (NOT TARGET fmt)
    FetchContent_Declare(fmt SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/fmt")
    FetchContent_MakeAvailable(fmt)
endif()


function(fp_add_benchmark NAME)
  cmake_parse_arguments(FPBM "" "" "SOURCES;INCLUDES;LIBRARIES" ${ARGN})
  add_executable(${NAME})
  target_include_directories(${NAME} PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> ${FPBM_INCLUDES})
  target_sources(${NAME} PRIVATE ${FPBM_SOURCES})
  target_link_libraries(${NAME} PRIVATE ${FPBM_LIBRARIES})
  target_compile_features(${NAME} PRIVATE cxx_std_17)
  # ---- MSVC Specifics ----
  if (MSVC)
      # No need to not generate PDB
      # /permissive- should be the default
      # The compilation will fail without /experimental:newLambdaProcessor
      # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
      target_compile_options(${NAME} PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
      target_link_options(${NAME} PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
      set_target_properties(${NAME} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
  endif()
  set_target_properties(${NAME} PROPERTIES FOLDER fp/benchmark)
endfunction()


# ---- Fixed-precision Benchmark ----

fp_add_benchmark(to_chars_fixed_precision_benchmark
    SOURCES
        include/to_chars_fixed_precision_benchmark.h
        source/to_chars_fixed_precision_benchmark.cpp
        source/fp_to_chars_fixed_precision.cpp
        source/ryu_fixed_precision.cpp
        source/fmt_fixed_precision.cpp
    LIBRARIES  
        ryu::ryu
        fmt::fmt
        jkj_fp::common
        jkj_fp::charconv)


# ---- Shortest-roundtrip Benchmark ----

fp_add_benchmark(to_chars_shortest_roundtrip_benchmark
    SOURCES
        include/to_chars_shortest_roundtrip_benchmark.h
        source/to_chars_shortest_roundtrip_benchmark.cpp
        source/fp_to_chars_shortest_roundtrip.cpp
        source/ryu_shortest_roundtrip.cpp
        source/grisu_exact_shortest_roundtrip.cpp
        source/schubfach_shortest_roundtrip.cpp
    LIBRARIES
        ryu::ryu
        grisu_exact::grisu_exact
        schubfach::schubfach
        jkj_fp::common
        jkj_fp::charconv)


# ---- Limited-precision from_chars Benchmark ----

fp_add_benchmark(from_chars_limited_precision_benchmark
    SOURCES
        include/from_chars_limited_precision_benchmark.h
        source/from_chars_limited_precision_benchmark.cpp
        source/fp_from_chars_limited_precision.cpp
        source/ryu_stof.cpp
    LIBRARIES
        ryu::ryu
        jkj_fp::common
        jkj_fp::charconv)


# ---- Unlimited-precision from_chars Benchmark ----

fp_add_benchmark(from_chars_unlimited_precision_benchmark
    SOURCES
        include/from_chars_unlimited_precision_benchmark.h
        source/from_chars_unlimited_precision_benchmark.cpp
        source/fp_from_chars_unlimited_precision.cpp
        source/std_from_chars.cpp
        source/stod.cpp
    LIBRARIES
        jkj_fp::common
        jkj_fp::charconv)
