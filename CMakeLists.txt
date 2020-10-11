
cmake_minimum_required(VERSION 3.15)
include(ExternalProject)

project(opentemple-native LANGUAGES CXX C)

#
# Set default properties
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(cmake/soloud.cmake)
include(cmake/libjpegturbo.cmake)
add_subdirectory(game)
add_subdirectory(thirdparty_wrappers)
add_subdirectory(ffmpeg_bink)

add_library(opentemple SHARED thirdparty_wrappers/soloud.def)
target_link_libraries(opentemple PRIVATE game thirdparty_wrappers)
set_target_properties(opentemple PROPERTIES OUTPUT_NAME "OpenTemple.Native")

add_subdirectory(managed)
