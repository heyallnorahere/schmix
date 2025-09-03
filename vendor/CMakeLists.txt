cmake_minimum_required(VERSION 3.21)

add_subdirectory("Coral")

find_package(SDL3 REQUIRED)

set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(IMGUI_DIR "${VENDOR_DIR}/imgui")
set(CIMGUI_DIR "${VENDOR_DIR}/cimgui")

set(IMNODES_DIR "${VENDOR_DIR}/imnodes")
set(CIMNODES_DIR "${VENDOR_DIR}/cimnodes")

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

file(GLOB IMNODES_SRC CONFIGURE_DEPENDS "${IMNODES_DIR}/imnodes*.*")
add_library(imnodes STATIC ${IMNODES_SRC})
target_link_libraries(imnodes PUBLIC imgui)
target_include_directories(imnodes PUBLIC ${IMNODES_DIR})

# fucking why dude
target_compile_definitions(imnodes PUBLIC IMNODES_NAMESPACE=imnodes)

set_target_properties(imgui imnodes PROPERTIES CXX_STANDARD 11)

macro(generate_c_bindings CPP_NAME BINDING_NAME MESSAGE)
    set(CPP_DIR "${VENDOR_DIR}/${CPP_NAME}")
    set(CPP_SRC "${CPP_DIR}/${CPP_NAME}.cpp")

    set(BINDING_DIR "${VENDOR_DIR}/${BINDING_NAME}")
    set(BINDING_SRC "${BINDING_DIR}/${BINDING_NAME}.cpp")
    set(GEN_DIR "${BINDING_DIR}/generator")

    file(GLOB TEMPLATE_SOURCE "${GEN_DIR}/${BINDING_NAME}*.*")
    add_custom_command(
        OUTPUT ${BINDING_SRC}
        COMMAND luajit "${GEN_DIR}/generator.lua" gcc "internal noimstrv"
        VERBATIM
        DEPENDS ${CPP_SRC} ${TEMPLATE_SOURCE}
        WORKING_DIRECTORY ${GEN_DIR}
        COMMENT ${MESSAGE}
    )
endmacro()

set(ENV{IMGUI_PATH} ${IMGUI_DIR})
generate_c_bindings(imgui cimgui "Generating Dear ImGui C bindings...")

set(ENV{IMNODES_PATH} ${IMNODES_DIR})
generate_c_bindings(imnodes cimnodes "Generating ImNodes C bindings...")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_library(cimgui SHARED "${CIMGUI_DIR}/cimgui.cpp")
target_link_libraries(cimgui PUBLIC imgui)
target_include_directories(cimgui PUBLIC ${CIMGUI_DIR})

add_library(cimnodes SHARED "${CIMNODES_DIR}/cimnodes.cpp")
target_link_libraries(cimnodes PUBLIC imnodes cimgui)
target_include_directories(cimnodes PUBLIC ${CIMNODES_DIR})

set(BINDINGS cimgui cimnodes)
set_target_properties(${BINDINGS} PROPERTIES CXX_STANDARD 11)

foreach(BINDING_NAME ${BINDINGS})
    add_custom_command(
        TARGET ${BINDING_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${BINDING_NAME}>
            "${CMAKE_CURRENT_SOURCE_DIR}/../assets/runtime/"
    )
endforeach()
