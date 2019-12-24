
cmake_minimum_required(VERSION 3.12)

include(ExternalProject)

project(opentemple-native LANGUAGES CXX C)

#
# Set default properties
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

#
# Build soloud as a static library externally
#
set(soloud_libfile "${CMAKE_CURRENT_BINARY_DIR}/soloud/lib/${CMAKE_STATIC_LIBRARY_PREFIX}soloud${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(soloud_includes "${CMAKE_CURRENT_BINARY_DIR}/soloud/include")
# CMake checks that INTERFACE_INCLUDE_DIRECTORIES exist at configure time :(
file(MAKE_DIRECTORY "${soloud_includes}")

# Create an imported target for the static library
add_library(soloud STATIC IMPORTED)
set_target_properties(soloud PROPERTIES
        IMPORTED_LOCATION ${soloud_libfile}
        INTERFACE_INCLUDE_DIRECTORIES ${soloud_includes}
        )

if (WIN32)
    set(SOLOUD_EXTRA_ARGS -DSOLOUD_BACKEND_SDL2:BOOL=OFF -DSOLOUD_BACKEND_WASAPI:BOOL=ON)
elseif (APPLE)
    set(SOLOUD_EXTRA_ARGS -DSOLOUD_BACKEND_SDL2:BOOL=OFF -DSOLOUD_BACKEND_COREAUDIO:BOOL=ON)
    set_target_properties(soloud PROPERTIES
            IMPORTED_LINK_INTERFACE_LIBRARIES "-framework CoreAudio -framework AudioToolbox"
            )
else ()
    # For whatever reason soloud uses lib64 on linux :-(
    set(soloud_libfile "${CMAKE_CURRENT_BINARY_DIR}/soloud/lib64/${CMAKE_STATIC_LIBRARY_PREFIX}soloud${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set_target_properties(soloud PROPERTIES IMPORTED_LOCATION ${soloud_libfile})
endif ()

ExternalProject_Add(soloud-ep
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/thirdparty/soloud/contrib
        BUILD_BYPRODUCTS ${soloud_libfile}
        CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/soloud
        ${SOLOUD_EXTRA_ARGS}
        )
add_dependencies(soloud soloud-ep)

#
# Build libjpeg-turbo as a static library externally
#
if (WIN32)
    # Use the nasm we included in the repo on windows
    set(LIBJPEG_TURBO_EXTRA_ARGS -DCMAKE_ASM_NASM_COMPILER:FILEPATH=${CMAKE_CURRENT_LIST_DIR}/tools-win/nasm.exe)
endif ()

# Sadly libjpeg-turbo uses different library names on windows
if (WIN32)
    set(turbojpeg_libname turbojpeg-static)
else ()
    set(turbojpeg_libname turbojpeg)
endif ()

set(libjpeg_turbo_libfile "${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${turbojpeg_libname}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(libjpeg_turbo_includes "${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo/include")
# CMake checks that INTERFACE_INCLUDE_DIRECTORIES exist at configure time :(
file(MAKE_DIRECTORY "${libjpeg_turbo_includes}")
ExternalProject_Add(libjpeg-turbo-ep
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/thirdparty/libjpeg-turbo
        BUILD_BYPRODUCTS ${libjpeg_turbo_libfile}
        CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DENABLE_SHARED:BOOL=FALSE
        -DREQUIRE_SIMD=TRUE
        -DWITH_ARITH_DEC=FALSE
        -DWITH_ARITH_ENC=FALSE
        -DWITH_CRT_DLL=TRUE
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo
        ${LIBJPEG_TURBO_EXTRA_ARGS}
        )

# Create an imported target for the static library
add_library(libjpeg-turbo STATIC IMPORTED)
set_target_properties(libjpeg-turbo PROPERTIES
        IMPORTED_LOCATION ${libjpeg_turbo_libfile}
        INTERFACE_INCLUDE_DIRECTORIES ${libjpeg_turbo_includes})
add_dependencies(libjpeg-turbo libjpeg-turbo-ep)

set(SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/libjpeg_turbo_wrapper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/stb_image_wrapper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/vector_wrapper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/string_wrapper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/soloud_c.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/soloud.def
        )
if (WIN32)
    set(SOURCES ${SOURCES}
            ${CMAKE_CURRENT_LIST_DIR}/src/install_dir_finder_win.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/install_dir_selector_win.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/windows_wrapper.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/game_folders_windows.cpp
            )
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif ()

add_library(spicytemple SHARED ${SOURCES})
set_target_properties(spicytemple PROPERTIES OUTPUT_NAME "SpicyTemple.Native")
target_include_directories(spicytemple PRIVATE ${CMAKE_CURRENT_LIST_DIR}/thirdparty/stb)
target_link_libraries(spicytemple libjpeg-turbo soloud)
