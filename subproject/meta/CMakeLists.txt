cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(jkj_fp_meta LANGUAGES CXX)

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

function(meta_exe NAME)
  add_executable(${NAME} source/${NAME}.cpp)

  target_compile_features(${NAME} PRIVATE cxx_std_17)

  target_link_libraries(${NAME} PRIVATE ${ARGN})

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
endfunction()

meta_exe(to_chars_fixed_precision_live_test jkj_fp::charconv)
meta_exe(to_chars_shortest_live_test jkj_fp::charconv)
meta_exe(from_chars_limited_precision_live_test ryu jkj_fp::charconv)
meta_exe(ryu_printf_generate_fast_cache jkj_fp::common)
meta_exe(dragonbox_generate_cache jkj_fp::common)
meta_exe(dragonbox_generate_compact_cache_error_table jkj_fp::common)
meta_exe(sandbox ryu fmt jkj_fp::charconv jkj_fp::common)