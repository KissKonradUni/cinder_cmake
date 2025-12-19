project(libcinder DESCRIPTION "The core static library for the engine's inner workings."
                      LANGUAGES CXX)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(libcinder STATIC ${APP_SOURCES})
target_include_directories(libcinder PUBLIC include)