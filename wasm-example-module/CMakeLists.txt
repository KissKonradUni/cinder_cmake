project(wasm_example_module DESCRIPTION "An example WebAssembly module using WAMR."
                            LANGUAGES CXX)

set(CMAKE_CXX_LINK_EXECUTABLE "wasm-ld --no-entry --export-all <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(wasm_example_module ${APP_SOURCES})
target_include_directories(wasm_example_module PUBLIC include)
target_include_directories(wasm_example_module PUBLIC include ${CMAKE_BINARY_DIR}/include)

# TODO: Modify this jank
include_directories(${CMAKE_SOURCE_DIR}/engine/include/wasm)

set_target_properties(wasm_example_module PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wasm_modules
    OUTPUT_NAME "wasm_example_module.wasm"
)

# Link cstind
set_target_properties(wasm_example_module PROPERTIES
    LINK_FLAGS "--import-memory -lcstind -lm"
)

target_compile_options(wasm_example_module PRIVATE
    --target=wasm32
)