cmake_minimum_required(VERSION 3.20)
project(convertdav LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DAVTOOLS_FETCH_BOOST "Download Boost headers when Boost is not installed" ON)
option(DAVTOOLS_BUILD_PANEL "Build the FTUI Windows control panel" ON)
option(DAVTOOLS_BUILD_FTHT_PANEL "Build the FTHt browser control panel" ON)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_path(BOOST_BEAST_INCLUDE_DIR boost/beast/http.hpp)
if (NOT BOOST_BEAST_INCLUDE_DIR AND DAVTOOLS_FETCH_BOOST)
    FetchContent_Declare(
        boost_headers
        URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
        URL_HASH SHA256=be0d91732d5b0cc6fbb275c7939974457e79b54d6f07ce2e3dfdd68bef883b0b
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(boost_headers)
    set(BOOST_BEAST_INCLUDE_DIR ${boost_headers_SOURCE_DIR})
endif()

if (NOT BOOST_BEAST_INCLUDE_DIR)
    message(FATAL_ERROR "Boost headers were not found. Install Boost or configure with -DDAVTOOLS_FETCH_BOOST=ON.")
endif()

set(DAVTOOLS_CORE_SOURCES
    src/server.cpp

    src/converters/common.cpp
    src/converters/registry.cpp
    src/converters/png_jpg.cpp
    src/converters/invert.cpp
    src/converters/img_gif.cpp
    src/converters/pdf_png.cpp
    src/converters/mp4_gif.cpp
    src/converters/virustest.cpp
    src/converters/md5.cpp
    src/converters/sha256.cpp
    src/converters/base64.cpp
    src/converters/json_min.cpp
    src/converters/base64_dec.cpp
)

add_executable(convertdav
    src/main.cpp
    ${DAVTOOLS_CORE_SOURCES}
)

function(configure_davtools_target target_name)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${BOOST_BEAST_INCLUDE_DIR}
    )

    # Boost.Asio/Beast without libboost_system
    target_compile_definitions(${target_name} PRIVATE
        BOOST_ERROR_CODE_HEADER_ONLY=1
    )

    target_link_libraries(${target_name} PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
    )

    if (WIN32)
        target_link_libraries(${target_name} PRIVATE ws2_32 mswsock)
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()
endfunction()

configure_davtools_target(convertdav)

if (WIN32 AND DAVTOOLS_BUILD_PANEL)
    add_executable(convertdav-panel WIN32
        src/control_panel.cpp
        ${DAVTOOLS_CORE_SOURCES}
    )
    configure_davtools_target(convertdav-panel)
    target_compile_definitions(convertdav-panel PRIVATE
        DAVTOOLS_FTUI_GUI=1
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        NTDDI_VERSION=0x0A000000
    )
    target_include_directories(convertdav-panel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ftui
    )
    target_link_libraries(convertdav-panel PRIVATE
        d2d1
        dwrite
        gdi32
        ole32
        uuid
        user32
        windowscodecs
    )
endif()

if (DAVTOOLS_BUILD_FTHT_PANEL)
    add_executable(convertdav-ftht-panel
        src/ftht_control_panel.cpp
        ${DAVTOOLS_CORE_SOURCES}
    )
    configure_davtools_target(convertdav-ftht-panel)
    target_compile_definitions(convertdav-ftht-panel PRIVATE
        DAVTOOLS_FTHT_PANEL=1
    )
    target_include_directories(convertdav-ftht-panel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ftht
    )
    if (WIN32)
        target_compile_definitions(convertdav-ftht-panel PRIVATE
            WINVER=0x0A00
            _WIN32_WINNT=0x0A00
            NTDDI_VERSION=0x0A000000
        )
        target_link_libraries(convertdav-ftht-panel PRIVATE shell32)
    endif()
endif()
