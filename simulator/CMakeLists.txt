cmake_minimum_required(VERSION 3.24)
project(blob_watch_simulator LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(FetchContent)
set(LVGL_SOURCE_DIR "" CACHE PATH "Optional existing LVGL 8.3.11 source tree (offline build)")
if(NOT LVGL_SOURCE_DIR)
    FetchContent_Declare(lvgl_source
        URL https://codeload.github.com/lvgl/lvgl/tar.gz/refs/tags/v8.3.11
        URL_HASH SHA256=83b7325a4b78cbb19a39f3ac75346c8086926d7faeab5ff2d825c20da0bbdb46
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR _download_only)
    FetchContent_MakeAvailable(lvgl_source)
    set(LVGL_SOURCE_DIR "${lvgl_source_SOURCE_DIR}")
endif()
file(READ "${LVGL_SOURCE_DIR}/lvgl.h" LVGL_HEADER)
if(NOT LVGL_HEADER MATCHES "LVGL_VERSION_MAJOR 8" OR
   NOT LVGL_HEADER MATCHES "LVGL_VERSION_MINOR 3" OR
   NOT LVGL_HEADER MATCHES "LVGL_VERSION_PATCH 11")
    message(FATAL_ERROR "The simulator requires LVGL 8.3.11, matching platformio.ini")
endif()
set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(APP "${APP_ROOT}/apps/blob_native")
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS "${LVGL_SOURCE_DIR}/src/*.c")
add_library(lvgl_sim STATIC ${LVGL_SOURCES})
target_compile_definitions(lvgl_sim PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_include_directories(lvgl_sim PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${APP}/lv_port" "${LVGL_SOURCE_DIR}")
add_executable(blob_simulator
    main.cpp runtime.cpp scenarios.cpp
    "${APP_ROOT}/platform/desktop/waveshare_desktop_board.cpp"
    "${APP}/lv_port/lv_port.cpp" "${APP}/lv_port/theme.cpp"
    "${APP}/pages/page_manager.cpp" "${APP}/pages/blob/blob_page.cpp"
    "${APP}/pages/timer/timer_page.cpp" "${APP}/pages/brightness/brightness_page.cpp"
    "${APP}/timer/runtime_draw.cpp" "${APP}/timer/logic.cpp" "${APP}/blob/shape.cpp" "${APP}/blob/face.cpp")
target_compile_definitions(blob_simulator PRIVATE BLOB_DESKTOP_SIMULATOR)
target_include_directories(blob_simulator PRIVATE "${APP_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(blob_simulator PRIVATE lvgl_sim SDL2::SDL2)
if(NOT MSVC)
    target_compile_options(blob_simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
add_test(NAME simulator_integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_simulator.py" "$<TARGET_FILE:blob_simulator>"
    "${CMAKE_CURRENT_BINARY_DIR}/screenshots")
add_test(NAME navigation_regression COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_page_navigation.py")

add_test(NAME timer_logic_regression COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_timer_logic.py")
