cmake_minimum_required(VERSION 3.21)
project(SimpleInjector VERSION 1.1.0 LANGUAGES CXX)

# Windows resource compiler: embeds the icon and version info into the .exe.
if(WIN32)
    enable_language(RC)
endif()

# Display version: v<major>.<minor> (for example v1.1).
set(SI_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Minimum Qt version; can be lowered for a trial build: -DSI_QT_MIN_VERSION=6.4
set(SI_QT_MIN_VERSION 6.7 CACHE STRING "Minimum Qt 6 version")

find_package(Qt6 ${SI_QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON) # compiles resources/app.qrc (the icon) into the executable

# Enables strict warnings on every target of the project.
function(si_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---- Source lists -----------------------------------------------------------
set(SI_CORE_SOURCES
    # core/io: JSON files and application data folder
    src/core/io/app_storage.cpp               src/core/io/app_storage.h
    src/core/io/json_io.cpp                   src/core/io/json_io.h
    # core/flags: the user's flags and their table model
    src/core/flags/flag_name.cpp              src/core/flags/flag_name.h
    src/core/flags/flag_value.cpp             src/core/flags/flag_value.h
    src/core/flags/flag_store.cpp             src/core/flags/flag_store.h
    src/core/flags/flag_table_model.cpp       src/core/flags/flag_table_model.h
    src/core/flags/flag_filter_proxy_model.cpp src/core/flags/flag_filter_proxy_model.h
    # core/offsets: the offsets table (parsing, lookup, cache, versions)
    src/core/offsets/offset_map.h
    src/core/offsets/offset_lookup.cpp        src/core/offsets/offset_lookup.h
    src/core/offsets/offsets_header_parser.cpp src/core/offsets/offsets_header_parser.h
    src/core/offsets/offsets_cache.cpp        src/core/offsets/offsets_cache.h
    src/core/offsets/roblox_version.cpp       src/core/offsets/roblox_version.h
    # core/apply: applying flags to a process
    src/core/apply/process_memory.h
    src/core/apply/flag_status.cpp            src/core/apply/flag_status.h
    src/core/apply/flag_applier.cpp           src/core/apply/flag_applier.h
    # platform: the Roblox process (Windows APIs)
    src/platform/process_probe.h
    src/platform/process_monitor.cpp          src/platform/process_monitor.h
    src/platform/roblox_process.cpp           src/platform/roblox_process.h
    # services: network access
    src/services/offsets_service.cpp          src/services/offsets_service.h
)
set(SI_APP_SOURCES
    src/main.cpp
    src/ui/add_flag_dialog.cpp src/ui/add_flag_dialog.h
    src/ui/main_window.cpp     src/ui/main_window.h
    src/ui/style.h
    src/ui/title_bar.cpp       src/ui/title_bar.h
    src/ui/window_sizing.h
    resources/app.qrc
)
set(SI_RESOURCE_FILES
    resources/icons/app.png
    resources/icons/app.ico
    resources/app.rc.in
)
set(SI_TEST_SOURCES
    tests/test_flag_name.cpp
    tests/test_flag_value.cpp
    tests/test_flag_store.cpp
    tests/test_flag_table_model.cpp
    tests/test_flag_applier.cpp
    tests/test_json_io.cpp
    tests/test_app_storage.cpp
    tests/test_offsets_parsing.cpp
    tests/test_offsets_cache.cpp
    tests/test_roblox_version.cpp
    tests/test_offsets_service.cpp
    tests/test_process_monitor.cpp
    tests/test_window_sizing.cpp
)

# ---- Sanity checks ----------------------------------------------------------
# Report ALL missing source files at once (instead of stopping at the first error).
set(_si_missing "")
foreach(_file IN LISTS SI_CORE_SOURCES SI_APP_SOURCES SI_RESOURCE_FILES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
        list(APPEND _si_missing "${_file}")
    endif()
endforeach()
if(BUILD_TESTING)
    foreach(_file IN LISTS SI_TEST_SOURCES)
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
            list(APPEND _si_missing "${_file}")
        endif()
    endforeach()
endif()
if(_si_missing)
    list(JOIN _si_missing "\n  " _si_missing_text)
    message(FATAL_ERROR
        "Missing source files (were they added and committed to the repository?):\n  ${_si_missing_text}")
endif()

# Guard against a half-updated checkout: each key file must contain a marker that only the
# current version has. Typical cause: a file pasted under the wrong name, or not replaced.
set(_si_stale "")
function(si_require_text file)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${file}" _content)
    foreach(_marker IN LISTS ARGN)
        string(FIND "${_content}" "${_marker}" _pos)
        if(_pos EQUAL -1)
            set(_si_stale "${_si_stale}\n  ${file}  (missing: ${_marker})" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()
si_require_text(src/core/flags/flag_name.h               "cleanPrefix")
si_require_text(src/core/offsets/offset_lookup.h         "lookupOffset")
si_require_text(src/core/offsets/offsets_header_parser.h "parseOffsetsHeader")
si_require_text(src/core/apply/flag_status.h             "isFailed")
si_require_text(src/core/apply/flag_applier.h            "ApplyReport")
si_require_text(src/core/flags/flag_table_model.h        "failedCount")
si_require_text(src/core/flags/flag_filter_proxy_model.h "setFailedOnly")
si_require_text(src/platform/roblox_process.h            "IProcessMemory")
si_require_text(src/services/offsets_service.h           "Source")
si_require_text(src/ui/main_window.h                     "failedOnlyCheck_")
si_require_text(src/ui/window_sizing.h                   "fitToScreen")
if(_si_stale)
    message(FATAL_ERROR
        "Some source files have WRONG or OUTDATED content (usually a file pasted under the wrong "
        "name, or not replaced by the new version):\n${_si_stale}\n\n"
        "Replace each file above with its latest version and run CMake again.")
endif()

# ---- Core library (no UI, unit-testable) ------------------------------------
add_library(SimpleInjectorCore STATIC ${SI_CORE_SOURCES})
target_link_libraries(SimpleInjectorCore PUBLIC Qt6::Core Qt6::Gui Qt6::Network)
target_include_directories(SimpleInjectorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
si_enable_warnings(SimpleInjectorCore)

# ---- Application ------------------------------------------------------------
qt_add_executable(SimpleInjector WIN32 ${SI_APP_SOURCES})
target_link_libraries(SimpleInjector PRIVATE SimpleInjectorCore Qt6::Widgets)
target_compile_definitions(SimpleInjector PRIVATE SI_APP_VERSION="${SI_VERSION_SHORT}")
si_enable_warnings(SimpleInjector)

if(WIN32)
    set(SI_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
    configure_file(resources/app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    target_sources(SimpleInjector PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")

    target_compile_definitions(SimpleInjector PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_definitions(SimpleInjectorCore PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ---- Tests ------------------------------------------------------------------
if(BUILD_TESTING)
    find_package(Qt6 ${SI_QT_MIN_VERSION} REQUIRED COMPONENTS Test)

    function(si_add_test name source)
        qt_add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE SimpleInjectorCore Qt6::Test)
        si_enable_warnings(${name})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    foreach(_source IN LISTS SI_TEST_SOURCES)
        get_filename_component(_name "${_source}" NAME_WE) # e.g. test_flag_store
        si_add_test(${_name} ${_source})
    endforeach()
endif()
