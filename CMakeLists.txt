
cmake_minimum_required(VERSION 3.15)

set(default_build_type "RelWithDebInfo")

# https://blog.kitware.com/cmake-and-the-default-build-type/
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "RelWithDebInfo")
endif ()

# The install command will prepare a directory for direct inclusion in the nuget package
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/nuget_files" CACHE PATH "Installation prefix" FORCE)

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

# Enable Debug Symbol Output (PDB) for release builds in MSVC
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif ()

if (RUNTIME_ID STREQUAL "")
    message(FATAL_ERROR "Unable to determine .NET Core Runtime ID")
endif ()

include(cmake/soloud.cmake)
include(cmake/libjpegturbo.cmake)
add_subdirectory(game)
add_subdirectory(thirdparty_wrappers)

add_library(opentemple SHARED thirdparty/soloud/src/c_api/soloud.def)
target_link_libraries(opentemple PRIVATE game thirdparty_wrappers)
set_target_properties(opentemple PROPERTIES OUTPUT_NAME "OpenTemple.Native")

install(FILES $<TARGET_PDB_FILE:opentemple> DESTINATION runtimes/${RUNTIME_ID}/native OPTIONAL)
install(FILES $<TARGET_FILE:opentemple> DESTINATION runtimes/${RUNTIME_ID}/native)
