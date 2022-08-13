
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(sdl_libsuffix d)
else()
    set(sdl_libsuffix)
endif()
set(sdl_libfile "${CMAKE_CURRENT_BINARY_DIR}/sdl2/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2${sdl_libsuffix}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(sdl_includes "${CMAKE_CURRENT_BINARY_DIR}/sdl2/include")
# CMake checks that INTERFACE_INCLUDE_DIRECTORIES exist at configure time :(
file(MAKE_DIRECTORY "${sdl_includes}")

# Create an imported target for the static library
add_library(SDL2::SDL2-static STATIC IMPORTED)
set_target_properties(SDL2::SDL2-static PROPERTIES
        IMPORTED_LOCATION ${sdl_libfile}
        INTERFACE_INCLUDE_DIRECTORIES ${sdl_includes}
        )

ExternalProject_Add(sdl-ep
        BUILD_BYPRODUCTS ${sdl_libfile}
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.0.22.tar.gz
        URL_HASH SHA256=826e83c7a602b2025647e93c6585908379179f68d479dfc1d9b03d2b9570c8d9
        # BUILD_BYPRODUCTS ${zlibng_libfile}
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/sdl2
        # Since we link the static lib into a shared lib...
        -DSDL_STATIC_PIC:BOOL=ON
        -DSDL_TEST:BOOL=OFF
        -DSDL_SHARED:BOOL=OFF
        -DSDL_STATIC:BOOL=ON
        -DSDL_OFFSCREEN:BOOL=ON
        -DSDL2_DISABLE_SDL2MAIN:BOOL=ON
        -DSDL_FORCE_STATIC_VCRT:BOOL=OFF
        )
