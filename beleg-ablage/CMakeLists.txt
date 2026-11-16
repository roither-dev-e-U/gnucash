# CMakeLists.txt — out-of-tree GnuCash module "Beleg-Ablage"
#
# Copyright (C) 2026 Georg Roither, roither.dev e.U.
# License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>
# This is free software: you are free to change and redistribute it.
# There is NO WARRANTY, to the extent permitted by law.
#
# Standalone build against an existing GnuCash source and build tree:
#
#   cmake -B build \
#     -DGNCCASH_SOURCE_DIR=/path/to/gnucash-source \
#     -DGNCCASH_BUILD_DIR=/path/to/gnucash-build \
#     -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build
#
# The result is libgncmod-beleg-ablage.so (Linux) resp.
# libgncmod-beleg-ablage.dll (Windows/MSYS2) in the build directory.

cmake_minimum_required(VERSION 3.16)
project(beleg-ablage
        VERSION 1.0.0
        DESCRIPTION "GnuCash out-of-tree module: Beleg-Ablage (receipt archive)"
        LANGUAGES C)

# ----------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------

set(GNCCASH_SOURCE_DIR "" CACHE PATH
    "Path to the GnuCash *source* tree (git checkout of gnucash)")
set(GNCCASH_BUILD_DIR  "" CACHE PATH
    "Path to the GnuCash *build* tree (configured GnuCash cmake build)")

if(NOT GNCCASH_SOURCE_DIR OR NOT GNCCASH_BUILD_DIR)
  message(FATAL_ERROR
    "Both GNCCASH_SOURCE_DIR and GNCCASH_BUILD_DIR must be set, e.g.\n"
    "  cmake -B build -DGNCCASH_SOURCE_DIR=~/src/gnucash "
    "-DGNCCASH_BUILD_DIR=~/build/gnucash")
endif()

if(NOT EXISTS "${GNCCASH_SOURCE_DIR}/libgnucash/engine/Transaction.h")
  message(FATAL_ERROR
    "${GNCCASH_SOURCE_DIR} does not look like a GnuCash source tree "
    "(libgnucash/engine/Transaction.h not found)")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0 gio-2.0)

find_program(GRESOURCE_COMPILER
  NAMES ${GLIB_COMPILE_RESOURCES_EXE} glib-compile-resources)
if(NOT GRESOURCE_COMPILER)
  message(FATAL_ERROR "glib-compile-resources not found (glib development tools)")
endif()

message(STATUS "glib-compile-resources: ${GRESOURCE_COMPILER}")

# ----------------------------------------------------------------------
# GResource bundle (auto-registering constructor, i.e. default
# --generate-source mode — NO --manual-register)
# ----------------------------------------------------------------------

set(BELEG_RESOURCE_C "${CMAKE_CURRENT_BINARY_DIR}/beleg-ablage-resources.c")

add_custom_command(
  OUTPUT "${BELEG_RESOURCE_C}"
  COMMAND "${GRESOURCE_COMPILER}"
          --generate-source
          --sourcedir "${CMAKE_CURRENT_SOURCE_DIR}/resources"
          --target "${BELEG_RESOURCE_C}"
          "${CMAKE_CURRENT_SOURCE_DIR}/resources/beleg-ablage.gresource.xml"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/beleg-ablage.gresource.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/gnc-plugin-beleg-ablage.ui"
  COMMENT "Compiling Beleg-Ablage gresource bundle")

# ----------------------------------------------------------------------
# The module itself
# ----------------------------------------------------------------------

set(BELEG_MODULE_SOURCES
  gncmod-beleg-ablage.c
  gnc-plugin-beleg-ablage.c
  dialog-beleg-ablage.c
  beleg-ablage-kvp.c
  beleg-ablage-prefs.c
  beleg-ablage-util.c
  "${BELEG_RESOURCE_C}")

set(BELEG_MODULE_HEADERS
  gnc-plugin-beleg-ablage.h
  dialog-beleg-ablage.h
  beleg-ablage-kvp.h
  beleg-ablage-prefs.h
  beleg-ablage-util.h)

add_library(gncmod-beleg-ablage MODULE ${BELEG_MODULE_SOURCES} ${BELEG_MODULE_HEADERS})

# GnuCash's module loader requires the file to start with "libgncmod-"
# and to carry the G_MODULE_SUFFIX.
set_target_properties(gncmod-beleg-ablage PROPERTIES
  PREFIX "libgncmod-"
  OUTPUT_NAME "beleg-ablage"
  C_STANDARD 11)

target_include_directories(gncmod-beleg-ablage PRIVATE
  "${GNCCASH_BUILD_DIR}/common"          # generated config.h
  "${GNCCASH_SOURCE_DIR}/common"
  "${GNCCASH_SOURCE_DIR}/libgnucash/core-utils"
  "${GNCCASH_SOURCE_DIR}/libgnucash/engine"
  "${GNCCASH_SOURCE_DIR}/libgnucash/app-utils"
  "${GNCCASH_SOURCE_DIR}/libgnucash/gnc-module"
  "${GNCCASH_SOURCE_DIR}/gnucash/gnome"
  "${GNCCASH_SOURCE_DIR}/gnucash/gnome-utils"
  "${GNCCASH_SOURCE_DIR}/gnucash/register/ledger-core"
  "${GNCCASH_SOURCE_DIR}/gnucash/register/register-core"
  "${GNCCASH_SOURCE_DIR}/gnucash/register/register-gnome")

target_link_directories(gncmod-beleg-ablage PRIVATE
  "${GNCCASH_BUILD_DIR}/lib"
  "${GNCCASH_BUILD_DIR}/lib/gnucash")

# GnuCash core libraries from the build tree. On Windows/MSYS2 the import
# libraries live as libgnc-*.dll.a in lib/; on Linux as libgnc-*.so.
foreach(gnc_lib gnc-core-utils gnc-engine gnc-app-utils gnc-module
                gnc-gnome-utils gnc-gnome)
  find_library(BELEG_${gnc_lib} ${gnc_lib}
    HINTS "${GNCCASH_BUILD_DIR}/lib" "${GNCCASH_BUILD_DIR}/lib/gnucash"
    NO_DEFAULT_PATH)
  if(NOT BELEG_${gnc_lib})
    message(FATAL_ERROR
      "Library ${gnc_lib} not found in ${GNCCASH_BUILD_DIR}/lib — "
      "is the GnuCash build tree fully built?")
  endif()
  list(APPEND BELEG_GNC_LIBRARIES "${BELEG_${gnc_lib}}")
endforeach()

target_link_libraries(gncmod-beleg-ablage PRIVATE
  ${BELEG_GNC_LIBRARIES}
  ${GTK3_LIBRARIES})

foreach(dir ${GTK3_INCLUDE_DIRS})
  target_include_directories(gncmod-beleg-ablage PRIVATE "${dir}")
endforeach()

# Export all symbols on Windows (no .def file needed for GCC/MinGW).
set_target_properties(gncmod-beleg-ablage PROPERTIES
  ENABLE_EXPORTS ON)

install(TARGETS gncmod-beleg-ablage
        LIBRARY DESTINATION lib/gnucash
        RUNTIME DESTINATION lib/gnucash)

# ----------------------------------------------------------------------
# Unit test for the pure logic (no GTK required)
# ----------------------------------------------------------------------

enable_testing()

add_executable(test-beleg-ablage-util
  tests/test-beleg-ablage-util.c
  beleg-ablage-util.c)

set_target_properties(test-beleg-ablage-util PROPERTIES C_STANDARD 11)

target_include_directories(test-beleg-ablage-util PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}")

foreach(dir ${GLIB_INCLUDE_DIRS})
  target_include_directories(test-beleg-ablage-util PRIVATE "${dir}")
endforeach()

target_link_libraries(test-beleg-ablage-util PRIVATE ${GLIB_LIBRARIES})

add_test(NAME beleg-ablage-util COMMAND test-beleg-ablage-util)
