cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(jkj_fp_tests LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---- Add dependencies ----

# We want warnings in tests
set(jkj_fp_INCLUDE_WITHOUT_SYSTEM ON CACHE INTERNAL "")

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

# ---- Tests ----

enable_testing()

# setup convenience umbrella targets
add_custom_target(fp-test-build)       # target for building all tests
add_custom_target(fp-test-run          # target for running all tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG> ${FP_CTEST_OPTIONS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS fp-test-build)
add_custom_target(fp-test-run-verbose  # target for running all tests in verbose mode
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG> ${FP_CTEST_OPTIONS} -VV
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS fp-test-build)
set_target_properties(fp-test-build PROPERTIES FOLDER fp/test)
set_target_properties(fp-test-run PROPERTIES FOLDER fp/test)
set_target_properties(fp-test-run-verbose PROPERTIES FOLDER fp/test)
if(NOT TARGET test)  # maybe this target is defined in a higher level project
  add_custom_target(test)
  set_target_properties(test PROPERTIES FOLDER fp/test)
endif()
add_dependencies(test fp-test-run)

function(fp_add_test NAME)
  cmake_parse_arguments(TEST "CHARCONV;RYU" "" "" ${ARGN})
  if(TEST_CHARCONV)
    set(jkj_fp jkj_fp::charconv)
  else()
    set(jkj_fp jkj_fp::fp)
  endif()
  add_executable(${NAME} source/${NAME}.cpp)
  add_dependencies(fp-test-build ${NAME})
  target_link_libraries(${NAME} PRIVATE ${jkj_fp} jkj_fp::common)
  if(TEST_RYU)
    target_link_libraries(${NAME} PRIVATE ryu::ryu)
  endif()
  target_compile_features(${NAME} PRIVATE cxx_std_17)
  add_test(NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}>)
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
  set_target_properties(${NAME} PROPERTIES FOLDER fp/test)
endfunction()

fp_add_test(test_bigint)
fp_add_test(test_minmax_euclid)
fp_add_test(test_policy_holder)
fp_add_test(verify_log_computation)
fp_add_test(dragonbox_test_all_shorter_interval_cases CHARCONV RYU)
fp_add_test(dragonbox_uniform_random_test CHARCONV RYU)
fp_add_test(dragonbox_verify_fast_multiplication)
fp_add_test(dragonbox_verify_magic_division)
fp_add_test(dragonbox_dooly_binary32_exhaustive_joint_test RYU)
fp_add_test(dragonbox_dooly_binary64_uniform_random_joint_test RYU)
fp_add_test(ryu_printf_uniform_random_test CHARCONV RYU)
fp_add_test(ryu_printf_dooly_uniform_random_joint_test CHARCONV RYU)
