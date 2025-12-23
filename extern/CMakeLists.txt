cmake_minimum_required(VERSION 4.1)
project(imgui)

file(GLOB APP_SOURCES CONFIGURE_DEPENDS
    ./imgui/*.cpp
    ./imgui/backends/imgui_impl_sdl3.cpp
    ./imgui/backends/imgui_impl_sdlgpu3.cpp
)

add_library(imgui STATIC ${APP_SOURCES})
target_include_directories(imgui PUBLIC ./imgui)
target_include_directories(imgui PUBLIC ./imgui/backends)
target_include_directories(imgui PUBLIC include ${CMAKE_BINARY_DIR}/include)

# WAMR configuration
# TODO: make it adjustable for CI/CD cross-compilation
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WAMR_BUILD_PLATFORM "linux" CACHE INTERNAL "wamr_build_platform")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WAMR_BUILD_PLATFORM "windows" CACHE INTERNAL "wamr_build_platform")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(WAMR_BUILD_PLATFORM "darwin" CACHE INTERNAL "wamr_build_platform")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()
set(WAMR_BUILD_TARGET "X86_64" CACHE INTERNAL "wamr_build_target")
set(WAMR_BUILD_INTERP 1 CACHE INTERNAL "wamr_build_interp")
set(WAMR_BUILD_FAST_INTERP 1 CACHE INTERNAL "wamr_build_fast_interp")
set(WAMR_BUILD_AOT 1 CACHE INTERNAL "wamr_build_aot")
set(WAMR_BUILD_LIBC_BUILTIN 1 CACHE INTERNAL "wamr_build_libc_builtin")
set(WAMR_BUILD_LIBC_WASI 1 CACHE INTERNAL "wamr_build_libc_wasi")
set(WAMR_BUILD_SIMD 1 CACHE INTERNAL "wamr_build_simd")
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-micro-runtime CACHE INTERNAL "wamr_root_dir")

include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib STATIC ${WAMR_RUNTIME_LIB_SOURCE})

# Include shared utilities if needed
include(${WAMR_ROOT_DIR}/core/shared/utils/uncommon/shared_uncommon.cmake)

# Configure SDL3 to build as shared library only
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)

# Add slang to the project
include(./slang.cmake)

# Add projects with own CMakeList
add_subdirectory(SDL)
add_subdirectory(reflect-cpp)