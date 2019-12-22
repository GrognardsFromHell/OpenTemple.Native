
cmake_minimum_required(VERSION 3.15)

project(nightly_installer CXX)

#
# Set default properties
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

set(SOURCES main.cpp app.manifest resource.h app.rc certificate.h os.h)

add_executable(InstallOpenTempleNightly WIN32 ${SOURCES})
target_link_libraries(InstallOpenTempleNightly PRIVATE comctl32.lib Crypt32.lib Cryptui.lib)
target_compile_definitions(InstallOpenTempleNightly PRIVATE UNICODE _UNICODE)
