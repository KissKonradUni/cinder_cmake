project(cinder_engine DESCRIPTION "The core static library for the engine's inner workings."
                      LANGUAGES CXX)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(cinder_engine STATIC ${APP_SOURCES})
target_include_directories(cinder_engine PUBLIC include)
target_include_directories(cinder_engine PUBLIC include ${CMAKE_BINARY_DIR}/include)

include_directories(${WAMR_ROOT_DIR}/core/iwasm/include)

# Add external libraries
target_link_libraries(cinder_engine PUBLIC imgui SDL3::SDL3 reflectcpp vmlib slang)