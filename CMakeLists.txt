cmake_minimum_required(VERSION 3.21)

project(schmix LANGUAGES C CXX)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    list(APPEND SCHMIX_PLATFORMS linux)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    list(APPEND SCHMIX_PLATFORMS osx)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    list(APPEND SCHMIX_PLATFORMS windows)
endif()

add_subdirectory("vendor")
add_subdirectory("src")
