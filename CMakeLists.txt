cmake_minimum_required(VERSION 3.21)
project(SimpleInjector VERSION 1.1.0 LANGUAGES CXX)

# Trình biên dịch tài nguyên Windows (để gắn icon và thông tin phiên bản vào .exe).
if(WIN32)
    enable_language(RC)
endif()

# Phiên bản hiển thị: v<major>.<minor> (ví dụ v1.1).
set(SI_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Phiên bản Qt tối thiểu; có thể hạ khi build thử: -DSI_QT_MIN_VERSION=6.4
set(SI_QT_MIN_VERSION 6.7 CACHE STRING "Minimum Qt 6 version")

find_package(Qt6 ${SI_QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON) # biên dịch resources/app.qrc (icon) vào file thực thi

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
    src/core/window_sizing.h
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
    resources/app.qrc
)
set(SI_RESOURCE_FILES
    resources/icons/app.png
    resources/icons/app.ico
    resources/app.rc.in
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
    tests/test_window_sizing.cpp
)

# Kiểm tra trước để báo MỘT LẦN toàn bộ file bị thiếu (thay vì dừng ở lỗi đầu tiên).
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
        "Thiếu file nguồn (chưa được thêm/commit vào repo?):\n  ${_si_missing_text}")
endif()

# Kiểm tra NỘI DUNG file nguồn: phát hiện file bị dán nhầm (ví dụ flag_utils.h nằm trong
# flag_applier.h) hoặc còn bản cũ, và báo bằng MỘT thông báo rõ ràng thay vì hàng trăm lỗi
# biên dịch dây chuyền. Mỗi file phải chứa các chuỗi đặc trưng của đúng phiên bản hiện tại.
# (Khi sửa tên/cấu trúc các thành phần này, nhớ cập nhật danh sách bên dưới.)
set(_si_wrong "")
macro(si_require_text file)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${file}" _si_content)
    foreach(_needle ${ARGN})
        string(FIND "${_si_content}" "${_needle}" _si_pos)
        if(_si_pos EQUAL -1)
            string(APPEND _si_wrong "\n  ${file}  (không thấy: ${_needle})")
        endif()
    endforeach()
endmacro()

si_require_text(src/core/flag_utils.h          "lookupOffset" "QString cleanPrefix")
si_require_text(src/core/flag_utils.cpp        "lookupOffset(" "parseOffsetsHeader(")
si_require_text(src/core/flag_applier.h        "class IMemoryWriter" "previewFlag" "enum class FlagStatus")
si_require_text(src/core/flag_applier.cpp      "ApplyReport::summary" "previewFlag(" "applyFlags(")
si_require_text(src/core/flag_store.h          "class FlagStore" "setValue")
si_require_text(src/core/flag_store.cpp        "FlagStore::merge" "FlagStore::setValue")
si_require_text(src/core/flag_table_model.h    "class FlagTableModel" "class FlagFilterProxyModel" "isPending")
si_require_text(src/core/flag_table_model.cpp  "FlagTableModel::setData" "FlagFilterProxyModel::filterAcceptsRow")
si_require_text(src/core/offsets_cache.h       "loadOffsetsCache" "using OffsetMap")
si_require_text(src/core/offsets_cache.cpp     "loadOffsetsCache(" "saveOffsetsCache(")
si_require_text(src/core/roblox_version.h      "compareVersions" "parseOffsetsVersion")
si_require_text(src/core/roblox_version.cpp    "compareVersions(" "versionFromImagePath(")
si_require_text(src/core/app_storage.h         "prepareStorage" "struct StoragePaths")
si_require_text(src/core/app_storage.cpp       "prepareStorage(")
si_require_text(src/core/window_sizing.h       "fitToScreen" "centeredIn")
si_require_text(src/platform/process_probe.h   "class IProcessProbe" "struct ProcessStatus")
si_require_text(src/platform/process_monitor.h "class ProcessMonitor" "IProcessProbe")
si_require_text(src/platform/process_monitor.cpp "ProcessMonitor::checkNow")
si_require_text(src/platform/roblox_process.h  "class RobloxProcess" "readInt32" "IMemoryWriter")
si_require_text(src/platform/roblox_process.cpp "RobloxProcess::refresh" "RobloxProcess::readInt32")
si_require_text(src/services/offsets_service.h "class OffsetsService" "lastError" "enum class Source")
si_require_text(src/services/offsets_service.cpp "OffsetsService::refresh" "OffsetsService::onReplyFinished")
si_require_text(src/main.cpp                   "int main(" "MainWindow")
si_require_text(src/ui/main_window.h           "class MainWindow" "refreshOffsetsButton_")
si_require_text(src/ui/main_window.cpp         "MainWindow::applyAll" "MainWindow::updateOffsetsLabel")
si_require_text(src/ui/title_bar.h             "class TitleBar")
si_require_text(src/ui/title_bar.cpp           "TitleBar::TitleBar")
si_require_text(src/ui/add_flag_dialog.h       "class AddFlagDialog")
si_require_text(src/ui/add_flag_dialog.cpp     "AddFlagDialog::AddFlagDialog")
si_require_text(src/ui/style.h                 "appStyleSheet" "statusStyle")
if(_si_wrong)
    message(FATAL_ERROR
        "Một số file nguồn có nội dung SAI hoặc CŨ (thường do dán nhầm file vào sai tên, "
        "hoặc chưa cập nhật bản mới):${_si_wrong}\n"
        "Hãy thay bằng đúng bản mới của từng file ở trên rồi chạy lại.")
endif()

# ---- Thư viện lõi (không có giao diện, kiểm thử được) -----------------------
add_library(SimpleInjectorCore STATIC ${SI_CORE_SOURCES})
target_link_libraries(SimpleInjectorCore PUBLIC Qt6::Core Qt6::Gui Qt6::Network)
target_include_directories(SimpleInjectorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
si_enable_warnings(SimpleInjectorCore)

# ---- Ứng dụng ---------------------------------------------------------------
qt_add_executable(SimpleInjector WIN32 ${SI_APP_SOURCES})
target_link_libraries(SimpleInjector PRIVATE SimpleInjectorCore Qt6::Widgets)
target_compile_definitions(SimpleInjector PRIVATE SI_APP_VERSION="${SI_VERSION_SHORT}")
si_enable_warnings(SimpleInjector)

if(WIN32)
    set(SI_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
    configure_file(resources/app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    target_sources(SimpleInjector PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

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
