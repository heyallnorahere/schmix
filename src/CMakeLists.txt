cmake_minimum_required(VERSION 3.21)

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
set(IMGUI_DIR "${VENDOR_DIR}/imgui")

find_package(SDL3 REQUIRED)
find_package(spdlog REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavutil libavformat)

foreach(PLATFORM ${SCHMIX_PLATFORMS})
    list(APPEND SCHMIX_DEFINES SCHMIX_PLATFORM_${PLATFORM})
endforeach()

file(
    GLOB_RECURSE SCHMIX_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(schmix ${SCHMIX_SRC})
target_include_directories(schmix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(schmix PROPERTIES CXX_STANDARD 20)
target_precompile_headers(schmix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/schmixpch.h")
target_compile_definitions(schmix PRIVATE ${SCHMIX_DEFINES} $<$<CONFIG:Debug>:SCHMIX_IS_DEBUG>)

target_link_libraries(
    schmix PRIVATE

    # submodules
    imgui
    imnodes
    Coral.Native
    libremidi::libremidi

    # system packages
    SDL3::SDL3
    spdlog::spdlog
    PkgConfig::FFMPEG
)
