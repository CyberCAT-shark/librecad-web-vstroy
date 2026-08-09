cmake_minimum_required(VERSION 3.28)
project(lccore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# LibreCAD repo root
if(NOT LCAD_ROOT)
    get_filename_component(LCAD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# Boost headers (header-only components)
find_path(Boost_INCLUDE_DIR
    NAMES boost/version.hpp
    HINTS
        "${BOOST_ROOT}"
        "$ENV{BOOST_ROOT}"
        "${Boost_ROOT}"
        "$ENV{Boost_ROOT}"
        "${BOOST_INCLUDEDIR}"
        "$ENV{BOOST_INCLUDEDIR}"
        /usr/local
        /usr
    PATH_SUFFIXES include
    NO_CMAKE_FIND_ROOT_PATH
)
if(NOT Boost_INCLUDE_DIR)
    message(FATAL_ERROR "Boost headers not found. Set BOOST_ROOT or Boost_INCLUDE_DIR to a directory containing boost/version.hpp.")
endif()

# The host's /usr/include contains both boost/ (which we want) and glibc
# headers (stdlib.h, math.h, ...) which must NOT be on the wasm include path
# because they would shadow emscripten's musl libc and fail to find
# bits/libc-header-start.h. Expose only the boost/ subdirectory by creating
# a clean directory holding a symlink to the real boost headers.
set(BoostCleanInclude "${CMAKE_BINARY_DIR}/boost_clean")
file(MAKE_DIRECTORY "${BoostCleanInclude}")
if(NOT EXISTS "${BoostCleanInclude}/boost")
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${Boost_INCLUDE_DIR}/boost" "${BoostCleanInclude}/boost")
endif()

# ---------------------------------------------------------------------------
# Source globs
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES       CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/engine/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/engine/*.h"
)
file(GLOB_RECURSE MATH_SOURCES         CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/math/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/math/*.h"
)
file(GLOB_RECURSE FILEIO_SOURCES       CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/fileio/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/fileio/*.h"
)
file(GLOB_RECURSE FILTERS_SOURCES      CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/filters/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/filters/*.h"
)
file(GLOB_RECURSE GENERATORS_SOURCES   CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/generators/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/generators/*.h"
)
file(GLOB_RECURSE GUI_SOURCES          CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/gui/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/gui/*.h"
)
file(GLOB_RECURSE DEBUG_SOURCES        CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/debug/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/debug/*.h"
)
file(GLOB_RECURSE INFORMATION_SOURCES  CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/information/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/information/*.h"
)
file(GLOB_RECURSE CREATION_SOURCES     CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/creation/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/creation/*.h"
)
file(GLOB_RECURSE MODIFICATION_SOURCES CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/modification/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/modification/*.h"
)
file(GLOB_RECURSE SCRIPTING_SOURCES    CONFIGURE_DEPENDS
    "${LCAD_ROOT}/librecad/src/lib/scripting/*.cpp"
    "${LCAD_ROOT}/librecad/src/lib/scripting/*.h"
)
file(GLOB_RECURSE LIBDXFRW_SOURCES     CONFIGURE_DEPENDS
    "${LCAD_ROOT}/libraries/libdxfrw/src/*.cpp"
    "${LCAD_ROOT}/libraries/libdxfrw/src/*.h"
)
file(GLOB_RECURSE MUPARSER_SOURCES     CONFIGURE_DEPENDS
    "${LCAD_ROOT}/libraries/muparser/src/*.cpp"
    "${LCAD_ROOT}/libraries/muparser/include/*.h"
)
file(GLOB_RECURSE JWWLIB_SOURCES       CONFIGURE_DEPENDS
    "${LCAD_ROOT}/libraries/jwwlib/src/*.cpp"
    "${LCAD_ROOT}/libraries/jwwlib/src/*.h"
)

set(ALL_CORE_SOURCES
    ${ENGINE_SOURCES}
    ${MATH_SOURCES}
    ${FILEIO_SOURCES}
    ${FILTERS_SOURCES}
    ${GENERATORS_SOURCES}
    ${GUI_SOURCES}
    ${DEBUG_SOURCES}
    ${INFORMATION_SOURCES}
    ${CREATION_SOURCES}
    ${MODIFICATION_SOURCES}
    ${SCRIPTING_SOURCES}
    ${LIBDXFRW_SOURCES}
    ${MUPARSER_SOURCES}
    ${JWWLIB_SOURCES}
)

# ---------------------------------------------------------------------------
# Exclusions
# ---------------------------------------------------------------------------

# 1) Test / fuzz files
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "/tests?/")
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "/fuzz/")

# 2) QtWidgets-dependent GUI files
#    render/widget/  — QWidget-based rendering (QApplication, QPaintDevice)
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "render/widget/")
#    no_used/        — legacy painter implementations
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "no_used/")
#    rs_graphicview  — inherits QWidget, pulls <QWidget>
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "rs_graphicview\\.")
#    lc_eventhandler — includes <QAction>, <QMouseEvent>
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "lc_eventhandler\\.")
#    rs_eventhandler — includes <QMouseEvent>
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "rs_eventhandler\\.")
#    lc_coordinates_parser — transitively pulls <QWidget> via rs_graphicview.h
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "lc_coordinates_parser\\.")

# 3) QFileDialog wrapper (fileio)
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "lc_filenameselectionservice\\.")

# 4) Settings exporter (engine/settings) — QFileDialog/QMessageBox UI only
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "lc_settingsexporter\\.")

# 5) Image exporter (generators/image) — requires QtSvg (QSvgGenerator)
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "lc_imageexporter\\.")

# 6) Legacy rs_filterdxf — depends on obsolete dl_dxf.h API (removed from
#    libdxfrw). The active filter is rs_filterdxfrw (libdxfrw.h).
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "rs_filterdxf\\.")

# 7) dl_jww-copy.cpp — stale duplicate of dl_jww.cpp referencing removed
#    DL_Jww members (readDxfGroups, processDXFGroup, getChoppedLine, ...).
list(FILTER ALL_CORE_SOURCES EXCLUDE REGEX "dl_jww-copy\\.cpp")

# ---------------------------------------------------------------------------
# Include directories (extracted from main CMakeLists.txt SHARED_INCLUDES)
# ---------------------------------------------------------------------------
set(LCAD_INCLUDES
    ${BoostCleanInclude}
    ${LCAD_ROOT}/libraries/jwwlib/src
    ${LCAD_ROOT}/libraries/libdxfrw/src
    ${LCAD_ROOT}/libraries/libdxfrw/src/intern
    ${LCAD_ROOT}/libraries/muparser/include
    ${LCAD_ROOT}/librecad/src/lib/actions
    ${LCAD_ROOT}/librecad/src/lib/creation
    ${LCAD_ROOT}/librecad/src/lib/debug
    ${LCAD_ROOT}/librecad/src/lib/engine
    ${LCAD_ROOT}/librecad/src/lib/engine/clipboard
    ${LCAD_ROOT}/librecad/src/lib/engine/document
    ${LCAD_ROOT}/librecad/src/lib/engine/document/blocks
    ${LCAD_ROOT}/librecad/src/lib/engine/document/container
    ${LCAD_ROOT}/librecad/src/lib/engine/document/dimstyles
    ${LCAD_ROOT}/librecad/src/lib/engine/document/dimstyles/builder
    ${LCAD_ROOT}/librecad/src/lib/engine/document/dimstyles/export
    ${LCAD_ROOT}/librecad/src/lib/engine/document/entities
    ${LCAD_ROOT}/librecad/src/lib/engine/document/entities/support
    ${LCAD_ROOT}/librecad/src/lib/engine/document/fonts
    ${LCAD_ROOT}/librecad/src/lib/engine/document/layers
    ${LCAD_ROOT}/librecad/src/lib/engine/document/patterns
    ${LCAD_ROOT}/librecad/src/lib/engine/document/textstyles
    ${LCAD_ROOT}/librecad/src/lib/engine/document/ucs
    ${LCAD_ROOT}/librecad/src/lib/engine/document/variables
    ${LCAD_ROOT}/librecad/src/lib/engine/document/views
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/angles_base
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/crosshair
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/highlight
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/info_cursor
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/overlay_box
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/preview
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/references
    ${LCAD_ROOT}/librecad/src/lib/engine/overlays/ucs_mark
    ${LCAD_ROOT}/librecad/src/lib/engine/settings
    ${LCAD_ROOT}/librecad/src/lib/engine/undo
    ${LCAD_ROOT}/librecad/src/lib/engine/utils
    ${LCAD_ROOT}/librecad/src/lib/fileio
    ${LCAD_ROOT}/librecad/src/lib/filters
    ${LCAD_ROOT}/librecad/src/lib/generators
    ${LCAD_ROOT}/librecad/src/lib/generators/image
    ${LCAD_ROOT}/librecad/src/lib/generators/layers
    ${LCAD_ROOT}/librecad/src/lib/generators/makercamsvg
    ${LCAD_ROOT}/librecad/src/lib/gui
    ${LCAD_ROOT}/librecad/src/lib/gui/grid
    ${LCAD_ROOT}/librecad/src/lib/gui/render
    ${LCAD_ROOT}/librecad/src/lib/gui/render/headless
    ${LCAD_ROOT}/librecad/src/lib/gui/render/widget
    ${LCAD_ROOT}/librecad/src/lib/information
    ${LCAD_ROOT}/librecad/src/lib/math
    ${LCAD_ROOT}/librecad/src/lib/modification
    ${LCAD_ROOT}/librecad/src/lib/printing
    ${LCAD_ROOT}/librecad/src/lib/scripting
)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------
add_library(lccore STATIC ${ALL_CORE_SOURCES})
target_include_directories(lccore PUBLIC ${LCAD_INCLUDES})
target_compile_definitions(lccore PUBLIC DWGSUPPORT MUPARSER_STATIC QC_APPDIR=librecad)
target_link_libraries(lccore PUBLIC Qt6::Core Qt6::Gui)

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
qt_add_executable(core_smoke
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/core_smoke.cpp
)
target_link_libraries(core_smoke PRIVATE lccore Qt6::Core Qt6::Gui)
target_include_directories(core_smoke PRIVATE ${LCAD_INCLUDES})

target_link_options(core_smoke PRIVATE
    "--embed-file ${CMAKE_CURRENT_SOURCE_DIR}/../tests/sample.dxf@/sample.dxf"
    "-sALLOW_MEMORY_GROWTH=1"
)
