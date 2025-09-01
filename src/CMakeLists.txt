cmake_minimum_required(VERSION 3.21)

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
set(IMGUI_DIR "${VENDOR_DIR}/imgui")

find_package(SDL3 REQUIRED)
find_package(spdlog REQUIRED)

file(GLOB IMGUI_BASE_SRC CONFIGURE_DEPENDS "${IMGUI_DIR}/*.h" "${IMGUI_DIR}/*.cpp")
file(GLOB IMGUI_MISC_SRC CONFIGURE_DEPENDS "${IMGUI_DIR}/misc/cpp/imgui_stdlib.*")

set(IMGUI_BACKENDS sdl3 sdlgpu3)
foreach(BACKEND_NAME ${IMGUI_BACKENDS})
    file(GLOB BACKEND_SOURCE "${IMGUI_DIR}/backends/imgui_impl_${BACKEND_NAME}.*")
    list(APPEND IMGUI_BACKEND_SRC ${BACKEND_SOURCE})
endforeach()

add_library(schmix_imgui STATIC ${IMGUI_BASE_SRC} ${IMGUI_MISC_SRC} ${IMGUI_BACKEND_SRC})
target_link_libraries(schmix_imgui PUBLIC SDL3::SDL3)
target_include_directories(schmix_imgui PUBLIC ${IMGUI_DIR})
set_target_properties(schmix_imgui PROPERTIES CXX_STANDARD 11)

file(GLOB_RECURSE SCHMIX_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(schmix ${SCHMIX_SRC})
target_link_libraries(schmix PRIVATE schmix_imgui SDL3::SDL3 Coral.Native spdlog::spdlog)
target_include_directories(schmix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(schmix PROPERTIES CXX_STANDARD 20)
target_precompile_headers(schmix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/schmixpch.h")
target_compile_definitions(schmix PRIVATE $<$<CONFIG:Debug>:SCHMIX_IS_DEBUG>)
