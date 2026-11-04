cmake_minimum_required(VERSION 3.21)
project(SimpleInjector VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Phiên bản Qt tối thiểu; có thể hạ khi build thử: -DSI_QT_MIN_VERSION=6.4
set(SI_QT_MIN_VERSION 6.7 CACHE STRING "Minimum Qt 6 version")

find_package(Qt6 ${SI_QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

# Hàm tiện ích: bật cảnh báo chặt cho mọi target của dự án.
function(si_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---- Danh sách file nguồn ---------------------------------------------------
set(SI_CORE_SOURCES
    src/core/app_storage.cpp        src/core/app_storage.h
    src/core/flag_applier.cpp       src/core/flag_applier.h
    src/core/flag_store.cpp         src/core/flag_store.h
    src/core/flag_table_model.cpp   src/core/flag_table_model.h
    src/core/flag_utils.cpp         src/core/flag_utils.h
    src/core/offsets_cache.cpp      src/core/offsets_cache.h
    src/core/roblox_version.cpp     src/core/roblox_version.h
    src/platform/process_monitor.cpp src/platform/process_monitor.h
    src/platform/process_probe.h
    src/platform/roblox_process.cpp src/platform/roblox_process.h
    src/services/offsets_service.cpp src/services/offsets_service.h
)
set(SI_APP_SOURCES
    src/main.cpp
    src/ui/add_flag_dialog.cpp src/ui/add_flag_dialog.h
    src/ui/main_window.cpp     src/ui/main_window.h
    src/ui/style.h
    src/ui/title_bar.cpp       src/ui/title_bar.h
)
set(SI_TEST_SOURCES
    tests/test_flag_utils.cpp
    tests/test_flag_store.cpp
    tests/test_flag_applier.cpp
    tests/test_storage.cpp
    tests/test_roblox_version.cpp
    tests/test_flag_table_model.cpp
    tests/test_offsets_service.cpp
    tests/test_process_monitor.cpp
)

# Kiểm tra trước để báo MỘT LẦN toàn bộ file bị thiếu (thay vì dừng ở lỗi đầu tiên).
set(_si_missing "")
foreach(_file IN LISTS SI_CORE_SOURCES SI_APP_SOURCES)
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
        "Thiếu file nguồn (chưa được thêm/commit vào repo?):\n  ${_si_missing_text}")
endif()

# ---- Thư viện lõi (không có giao diện, kiểm thử được) -----------------------
add_library(SimpleInjectorCore STATIC ${SI_CORE_SOURCES})
target_link_libraries(SimpleInjectorCore PUBLIC Qt6::Core Qt6::Network)
target_include_directories(SimpleInjectorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
si_enable_warnings(SimpleInjectorCore)

# ---- Ứng dụng ---------------------------------------------------------------
qt_add_executable(SimpleInjector WIN32 ${SI_APP_SOURCES})
target_link_libraries(SimpleInjector PRIVATE SimpleInjectorCore Qt6::Widgets)
si_enable_warnings(SimpleInjector)

if(WIN32)
    target_compile_definitions(SimpleInjector PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_definitions(SimpleInjectorCore PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ---- Kiểm thử ---------------------------------------------------------------
if(BUILD_TESTING)
    find_package(Qt6 ${SI_QT_MIN_VERSION} REQUIRED COMPONENTS Test)

    function(si_add_test name source)
        qt_add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE SimpleInjectorCore Qt6::Test)
        si_enable_warnings(${name})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    foreach(_source IN LISTS SI_TEST_SOURCES)
        get_filename_component(_name "${_source}" NAME_WE) # test_flag_utils
        si_add_test(${_name} ${_source})
    endforeach()
endif()
