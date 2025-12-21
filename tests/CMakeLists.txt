Include(FetchContent)

project(cinder_tests LANGUAGES CXX)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(cinder_tests ${APP_SOURCES})
target_link_libraries(cinder_tests PRIVATE Catch2::Catch2WithMain cinder_engine)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(cinder_tests)