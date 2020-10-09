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


# ---- Fixed-precision Benchmark ----

add_executable(to_chars_fixed_precision_benchmark
        include/to_chars_fixed_precision_benchmark.h
        source/to_chars_fixed_precision_benchmark.cpp
        source/fp_to_chars_fixed_precision.cpp
        source/ryu_fixed_precision.cpp
        source/fmt_fixed_precision.cpp)

target_compile_features(to_chars_fixed_precision_benchmark PRIVATE cxx_std_17)

target_include_directories(to_chars_fixed_precision_benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(to_chars_fixed_precision_benchmark
        PRIVATE
        ryu::ryu
        fmt::fmt
        jkj_fp::common
        jkj_fp::charconv)

if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(to_chars_fixed_precision_benchmark PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
    target_link_options(to_chars_fixed_precision_benchmark PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
    set_target_properties(to_chars_fixed_precision_benchmark PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()


# ---- Shortest-roundtrip Benchmark ----

add_executable(to_chars_shortest_roundtrip_benchmark
        include/to_chars_shortest_roundtrip_benchmark.h
        source/to_chars_shortest_roundtrip_benchmark.cpp
        source/fp_to_chars_shortest_roundtrip.cpp
        source/ryu_shortest_roundtrip.cpp
        source/grisu_exact_shortest_roundtrip.cpp
        source/schubfach_shortest_roundtrip.cpp)

target_compile_features(to_chars_shortest_roundtrip_benchmark PRIVATE cxx_std_17)

target_include_directories(to_chars_shortest_roundtrip_benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(to_chars_shortest_roundtrip_benchmark
        PRIVATE
        ryu::ryu
        grisu_exact::grisu_exact
        schubfach::schubfach
        jkj_fp::common
        jkj_fp::charconv)

if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(to_chars_shortest_roundtrip_benchmark PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
    target_link_options(to_chars_shortest_roundtrip_benchmark PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
    set_target_properties(to_chars_shortest_roundtrip_benchmark PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()


# ---- Limited-precision from_chars Benchmark ----

add_executable(from_chars_limited_precision_benchmark
        include/from_chars_limited_precision_benchmark.h
        source/from_chars_limited_precision_benchmark.cpp
        source/fp_from_chars_limited_precision.cpp
        source/ryu_stof.cpp)

target_compile_features(from_chars_limited_precision_benchmark PRIVATE cxx_std_17)

target_include_directories(from_chars_limited_precision_benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(from_chars_limited_precision_benchmark
        PRIVATE
        ryu::ryu
        jkj_fp::common
        jkj_fp::charconv)

if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(from_chars_limited_precision_benchmark PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
    target_link_options(from_chars_limited_precision_benchmark PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
    set_target_properties(from_chars_limited_precision_benchmark PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()


# ---- Unlimited-precision from_chars Benchmark ----

add_executable(from_chars_unlimited_precision_benchmark
        include/from_chars_unlimited_precision_benchmark.h
        source/from_chars_unlimited_precision_benchmark.cpp
        source/fp_from_chars_unlimited_precision.cpp
        source/std_from_chars.cpp
        source/stod.cpp)

target_compile_features(from_chars_unlimited_precision_benchmark PRIVATE cxx_std_17)

target_include_directories(from_chars_unlimited_precision_benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(from_chars_unlimited_precision_benchmark
        PRIVATE
        jkj_fp::common
        jkj_fp::charconv)

if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(from_chars_unlimited_precision_benchmark PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
    target_link_options(from_chars_unlimited_precision_benchmark PUBLIC $<$<CONFIG:Release>:/LTCG> /DEBUG:FASTLINK)
    set_target_properties(from_chars_unlimited_precision_benchmark PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()