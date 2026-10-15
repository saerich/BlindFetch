cmake_minimum_required(VERSION 3.16)

project(BlindFetchLoadTest C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


FILE(GLOB app_sources CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/compat/*.c
    ${CMAKE_SOURCE_DIR}/../Firmware/lib/net/HTTP.c
    ${CMAKE_SOURCE_DIR}/../Firmware/lib/OPAQUE/*.c
    ${CMAKE_SOURCE_DIR}/../Firmware/src/BlindFetch.c
    ${CMAKE_SOURCE_DIR}/../Firmware/components/noise_xk/src/*.c
    ${CMAKE_SOURCE_DIR}/../Firmware/components/liboprf/oprf/*.c
    ${CMAKE_SOURCE_DIR}/../Firmware/components/liboprf/oprf/aux_/*.c
    ${CMAKE_SOURCE_DIR}/../Firmware/components/libopaque/aux_/*.c
    ${CMAKE_SOURCE_DIR}/../Firmware/components/libopaque/*.c
    ${CMAKE_SOURCE_DIR}/../Interop/Interop.c
    ${CMAKE_SOURCE_DIR}/../Interop/OpaqueInterop.c
    ${CMAKE_SOURCE_DIR}/../Interop/OPRFInterop.c
)

list(FILTER app_sources EXCLUDE REGEX ".*/jni\\.c$")

add_executable(BlindFetchLoadTest ${app_sources})

target_include_directories(BlindFetchLoadTest PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compat
    ${CMAKE_SOURCE_DIR}/../Firmware/lib/net
    ${CMAKE_SOURCE_DIR}/../Firmware/lib/OPAQUE
    ${CMAKE_SOURCE_DIR}/../Firmware/src
    ${CMAKE_SOURCE_DIR}/../Firmware/components/libopaque
    ${CMAKE_SOURCE_DIR}/../Firmware/components/liboprf
    ${CMAKE_SOURCE_DIR}/../Firmware/components/liboprf/oprf
    ${CMAKE_SOURCE_DIR}/../Firmware/components/noise_xk/include
    ${CMAKE_SOURCE_DIR}/../Firmware/components/karmel/krml
    ${CMAKE_SOURCE_DIR}/../Firmware/components/karmel
    ${CMAKE_SOURCE_DIR}/../Firmware/components/karmel/minimal
    ${CMAKE_SOURCE_DIR}/../Interop
)

find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)

pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

target_link_libraries(BlindFetchLoadTest PRIVATE
    PkgConfig::CJSON
    CURL::libcurl
    sodium
)
target_compile_options(BlindFetchLoadTest PRIVATE
    "SHELL:-include ${CMAKE_SOURCE_DIR}/compat/freertos/task.h"
    "SHELL:-include string.h"
)
target_compile_definitions(BlindFetchLoadTest PRIVATE
    NATIVE_BUILD=1
)