cmake_minimum_required(VERSION 3.21)

add_subdirectory("Coral")

find_package(SDL3 REQUIRED)

set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(IMGUI_DIR "${VENDOR_DIR}/imgui")
set(CIMGUI_DIR "${VENDOR_DIR}/cimgui")

file(GLOB IMGUI_BASE_SRC CONFIGURE_DEPENDS "${IMGUI_DIR}/*.h" "${IMGUI_DIR}/*.cpp")
file(GLOB IMGUI_MISC_SRC CONFIGURE_DEPENDS "${IMGUI_DIR}/misc/cpp/imgui_stdlib.*")

set(IMGUI_BACKENDS sdl3 sdlgpu3)
foreach(BACKEND_NAME ${IMGUI_BACKENDS})
    file(GLOB BACKEND_SOURCE "${IMGUI_DIR}/backends/imgui_impl_${BACKEND_NAME}.*")
    list(APPEND IMGUI_BACKEND_SRC ${BACKEND_SOURCE})
endforeach()

add_library(imgui STATIC ${IMGUI_BASE_SRC} ${IMGUI_MISC_SRC} ${IMGUI_BACKEND_SRC})
target_link_libraries(imgui PUBLIC SDL3::SDL3)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
set_target_properties(imgui PROPERTIES CXX_STANDARD 11)

set(ENV{IMGUI_PATH} ${IMGUI_DIR})
set(GEN_DIR "${CIMGUI_DIR}/generator")
file(GLOB TEMPLATE_SOURCE CONFIGURE_DEPENDS "${GEN_DIR}/*.cpp" "${GEN_DIR}/*.h")
add_custom_command(
    OUTPUT "${CIMGUI_DIR}/cimgui.cpp"
    COMMAND luajit "${GEN_DIR}/generator.lua" gcc "internal noimstrv"
    VERBATIM
    DEPENDS "${IMGUI_DIR}/imgui.cpp" ${TEMPLATE_SOURCE}
    WORKING_DIRECTORY ${GEN_DIR}
    COMMENT "Generating Dear ImGui C bindings..."
)

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_library(cimgui SHARED "${CIMGUI_DIR}/cimgui.cpp")
target_link_libraries(cimgui PRIVATE imgui)

set(IMGUI_EXPORT "extern \"C\"")
if(WIN32)
    set(IMGUI_EXPORT "${IMGUI_EXPORT} __declspec(dllexport)")
    target_link_libraries(cimgui imm32)
endif()

target_compile_definitions(cimgui PUBLIC IMGUI_IMPL_API=${IMGUI_EXPORT})

target_include_directories(
    cimgui
    PUBLIC ${CIMGUI_DIR}
    PRIVATE ${IMGUI_DIR}
)

set_target_properties(
    cimgui PROPERTIES
    CXX_STANDARD 11
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../assets/runtime/"
)
