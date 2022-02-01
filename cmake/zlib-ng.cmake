
#
# Build soloud as a static library externally
#
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(zlibng_libsuffix d)
else()
    set(zlibng_libsuffix)
endif()
set(zlibng_libfile "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zlibstatic-ng${zlibng_libsuffix}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(zlibng_includes "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng/include")

## CMake checks that INTERFACE_INCLUDE_DIRECTORIES exist at configure time :(
file(MAKE_DIRECTORY "${zlibng_includes}")
#
## Create an imported target for the static library
add_library(zlib-ng STATIC IMPORTED)
set_target_properties(zlib-ng PROPERTIES
        IMPORTED_LOCATION ${zlibng_libfile}
        INTERFACE_INCLUDE_DIRECTORIES "${zlibng_includes}"
        )

ExternalProject_Add(zlib-ng-ep
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/zlib-ng
        BUILD_BYPRODUCTS ${zlibng_libfile}
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/zlib-ng
        -DWITH_GZFILEOP=OFF
        -DZLIB_ENABLE_TESTS=OFF
        -DINSTALL_UTILS=OFF
        )
add_dependencies(zlib-ng zlib-ng-ep)
