project(cinder_runtime DESCRIPTION "The runtime responsible for packaging standalone executables."
                       LANGUAGES CXX)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(cinder_runtime ${APP_SOURCES})
target_include_directories(cinder_runtime PUBLIC include)
target_include_directories(cinder_runtime PUBLIC include ${CMAKE_BINARY_DIR}/include)

# Add wamr includes
include_directories(${WAMR_ROOT_DIR}/core/iwasm/include)

target_link_libraries(cinder_runtime PUBLIC cinder_engine)

# Ensure assets are available next to the runtime binary for all launch modes
add_custom_command(TARGET cinder_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE_DIR:cinder_runtime>/assets
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:cinder_runtime>/assets
    COMMENT "Symlinking assets directory next to cinder_runtime"
    VERBATIM
)