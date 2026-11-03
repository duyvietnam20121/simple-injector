cmake_minimum_required(VERSION 3.21)
project(SimpleInjector VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

add_library(SimpleInjectorCore STATIC
    flag_utils.cpp
    flag_utils.h
)
target_link_libraries(SimpleInjectorCore PUBLIC Qt6::Core)
target_include_directories(SimpleInjectorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(SimpleInjector
    main.cpp
)
target_link_libraries(SimpleInjector PRIVATE
    SimpleInjectorCore
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_compile_definitions(SimpleInjector PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(MSVC)
    target_compile_options(SimpleInjectorCore PRIVATE /W4 /permissive-)
    target_compile_options(SimpleInjector PRIVATE /W4 /permissive-)
else()
    target_compile_options(SimpleInjectorCore PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(SimpleInjector PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    find_package(Qt6 6.7 REQUIRED COMPONENTS Test)

    qt_add_executable(SimpleInjectorTests
        tests/test_flag_utils.cpp
    )
    target_link_libraries(SimpleInjectorTests PRIVATE
        SimpleInjectorCore
        Qt6::Test
        Qt6::Core
    )

    if(MSVC)
        target_compile_options(SimpleInjectorTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(SimpleInjectorTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME SimpleInjector.UnitTests COMMAND SimpleInjectorTests)
endif()
