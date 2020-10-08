cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(jkj_fp
        VERSION 0.0.1
        LANGUAGES CXX)

# ---- Includes ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(jkj_fp_warning_guard "SYSTEM")
if(jkj_fp_INCLUDE_WITHOUT_SYSTEM)
  set(jkj_fp_warning_guard "")
endif()

# ---- Declare library (fp) ----

add_library(jkj_fp INTERFACE)
add_library(jkj_fp::fp ALIAS jkj_fp)

set(jkj_fp_headers
        include/jkj/fp/detail/bits.h
        include/jkj/fp/detail/div.h
        include/jkj/fp/detail/dragonbox_cache.h
        include/jkj/fp/detail/log.h
        include/jkj/fp/detail/macros.h
        include/jkj/fp/detail/ryu_printf_compact_cache.h
        include/jkj/fp/detail/ryu_printf_fast_cache.h
        include/jkj/fp/detail/undef_macros.h
        include/jkj/fp/detail/util.h
        include/jkj/fp/detail/wuint.h
        include/jkj/fp/decimal_fp.h
        include/jkj/fp/dooly.h
        include/jkj/fp/dragonbox.h
        include/jkj/fp/ieee754_format.h
        include/jkj/fp/policy.h
        include/jkj/fp/ryu_printf.h)

target_include_directories(jkj_fp
        ${jkj_fp_warning_guard}
        INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

target_compile_features(jkj_fp INTERFACE cxx_std_17)

# ---- Declare library (charconv) ----

set(jkj_fp_charconv_headers
        ${jkj_fp_headers}
        include/jkj/fp/to_chars/fixed_precision.h
        include/jkj/fp/to_chars/shortest_roundtrip.h
        include/jkj/fp/to_chars/to_chars_common.h)

set(jkj_fp_charconv_sources source/to_chars/to_chars.cpp)

add_library(jkj_fp_charconv STATIC
        ${jkj_fp_charconv_headers}
        ${jkj_fp_charconv_sources})
add_library(jkj_fp::charconv ALIAS jkj_fp_charconv)

target_include_directories(jkj_fp_charconv
        ${jkj_fp_warning_guard}
        PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

target_compile_features(jkj_fp_charconv PUBLIC cxx_std_17)

# ---- Install ----

option(JKJ_FP_INSTALL_CHARCONV
        "When invoked with --install, to_chars/from_chars families are installed together with the core library"
        On)

set(jkj_fp_directory "fp-${PROJECT_VERSION}")
set(jkj_fp_include_directory "${CMAKE_INSTALL_INCLUDEDIR}/${jkj_fp_directory}")
set(jkj_fp_install_targets jkj_fp)

if (JKJ_FP_INSTALL_CHARCONV)
  set(jkj_fp_install_targets ${jkj_fp_targets} jkj_fp_charconv)
endif()

install(TARGETS ${jkj_fp_install_targets}
        EXPORT jkj_fpTargets
        ARCHIVE #
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT jkj_fp_Development
        INCLUDES #
        DESTINATION "${jkj_fp_include_directory}")

set(jkj_fp_install_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/${jkj_fp_directory}")

write_basic_package_version_file(
        jkj_fpConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

install(EXPORT jkj_fpTargets
        NAMESPACE jkj_fp::
        DESTINATION "${jkj_fp_install_cmakedir}")

install(FILES
        "${PROJECT_SOURCE_DIR}/cmake/jkj_fpConfig.cmake"
        "${PROJECT_BINARY_DIR}/jkj_fpConfigVersion.cmake"
        DESTINATION "${jkj_fp_install_cmakedir}")

install(DIRECTORY include/jkj/fp/detail
        DESTINATION "${jkj_fp_include_directory}/jkj/fp")
install(FILES   include/jkj/fp/decimal_fp.h
                include/jkj/fp/dragonbox.h
                include/jkj/fp/ieee754_format.h
                include/jkj/fp/ryu_printf.h
        DESTINATION "${jkj_fp_include_directory}/jkj/fp")

if (JKJ_FP_INSTALL_CHARCONV)
  install(FILES ${jkj_fp_charconv_headers}
          DESTINATION "${jkj_fp_include_directory}/jkj/fp")
endif()

# ---- Subproject ----

option(JKJ_FP_ENABLE_SUBPROJECT "Build subproject as well" OFF)

if (JKJ_FP_ENABLE_SUBPROJECT)
    add_subdirectory("subproject/benchmark")
    add_subdirectory("subproject/meta")
    add_subdirectory("subproject/test")
endif()

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    target_compile_options(jkj_fp INTERFACE
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>)
    target_compile_options(jkj_fp_charconv PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
endif()