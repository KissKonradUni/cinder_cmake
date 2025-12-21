cmake_minimum_required(VERSION 4.1)

project(cinder VERSION 0.1
               DESCRIPTION "A 3D game engine built in modern C++"
               LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define build types, default to Release
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
else()
  message(STATUS "Build type: '${CMAKE_BUILD_TYPE}'")
endif()

# Define flto
include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result AND CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Enabling LTO")
    add_executable(foo IMPORTED)
    set_target_properties(foo PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "LTO not supported or not a Release build")
endif()

# Generate version.hpp from template
configure_file(version.hpp.in ${CMAKE_BINARY_DIR}/include/version.hpp @ONLY)
configure_file(imgui_shaders.hpp.in ${CMAKE_BINARY_DIR}/include/imgui_shaders.hpp @ONLY)

# Add subdirectories
add_subdirectory(engine)
add_subdirectory(runtime)
# add_subdirectory(editor)
add_subdirectory(tests)

# Add example wasm module
add_subdirectory(wasm-example-module)

# Add external libraries
add_subdirectory(extern)