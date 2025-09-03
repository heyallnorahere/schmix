cmake_minimum_required(VERSION 3.21)

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
set(IMGUI_DIR "${VENDOR_DIR}/imgui")

find_package(SDL3 REQUIRED)
find_package(spdlog REQUIRED)

file(GLOB_RECURSE SCHMIX_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(schmix ${SCHMIX_SRC})
target_link_libraries(schmix PRIVATE imgui SDL3::SDL3 Coral.Native spdlog::spdlog)
target_include_directories(schmix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(schmix PROPERTIES CXX_STANDARD 20)
target_precompile_headers(schmix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/schmixpch.h")
target_compile_definitions(schmix PRIVATE $<$<CONFIG:Debug>:SCHMIX_IS_DEBUG>)
