
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(sdl_libsuffix d)
else()
    set(sdl_libsuffix)
endif()
set(sdl_libfilename ${CMAKE_STATIC_LIBRARY_PREFIX}SDL2-static${sdl_libsuffix}${CMAKE_STATIC_LIBRARY_SUFFIX})
set(sdl_libfile "${CMAKE_CURRENT_BINARY_DIR}/sdl2/lib/${sdl_libfilename}")
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
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/prerelease-2.23.1.tar.gz
        URL_HASH SHA256=e41fc5aa206fd79bf38edf5b0a38abce58d021625a19bc7d2017f3cfaad6ed67
        PATCH_COMMAND ${CMAKE_COMMAND} -E echo "target_compile_definitions(SDL2-static PRIVATE DLL_EXPORT)" >> <SOURCE_DIR>/CMakeLists.txt
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
        -DSDL_LIBC:BOOL=ON

        # Disable all the SDL subsystems except for VIDEO
        -DSDL_ATOMIC=OFF
        -DSDL_AUDIO=OFF # might be good as soloud backend
        -DSDL_RENDER=OFF
        -DSDL_JOYSTICK=OFF
        -DSDL_HAPTIC=OFF
        -DSDL_HIDAPI=OFF
        -DSDL_POWER=OFF
        -DSDL_THREADS=OFF
        -DSDL_TIMERS=OFF
        -DSDL_FILE=OFF
        -DSDL_CPUINFO=OFF
        -DSDL_FILESYSTEM=OFF
        -DSDL_SENSOR=OFF
        -DSDL_LOCALE=OFF
        -DSDL_MISC=OFF
        )
add_dependencies(SDL2::SDL2-static sdl-ep)
target_link_options(SDL2::SDL2-static INTERFACE /WHOLEARCHIVE:${sdl_libfilename})
target_link_libraries(SDL2::SDL2-static INTERFACE user32;gdi32;winmm;imm32;ole32;oleaut32;version;uuid;advapi32;setupapi;shell32;dinput8)

