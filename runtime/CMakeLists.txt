project(cinder_runtime DESCRIPTION "The runtime responsible for packaging standalone executables."
                       LANGUAGES CXX)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(cinder_runtime ${APP_SOURCES})
target_include_directories(cinder_runtime PUBLIC include)
target_include_directories(cinder_runtime PUBLIC include ${CMAKE_BINARY_DIR}/include)

target_link_libraries(cinder_runtime PUBLIC cinder_engine)