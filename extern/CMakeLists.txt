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

# Configure SDL3 to build as shared library only
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)

# Add projects with own CMakeList
add_subdirectory(SDL)
add_subdirectory(reflect-cpp)