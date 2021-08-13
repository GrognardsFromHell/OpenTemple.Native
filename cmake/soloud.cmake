
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
    set(SOLOUD_EXTRA_ARGS
        -DSOLOUD_BACKEND_SDL2:BOOL=OFF
        -DSOLOUD_BACKEND_WASAPI:BOOL=ON
        -DSOLOUD_BACKEND_XAUDIO2:BOOL=ON
        -DSOLOUD_BACKEND_WINMM:BOOL=ON
        -DSOLOUD_C_API:BOOL=ON
    )
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
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/soloud/contrib
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
