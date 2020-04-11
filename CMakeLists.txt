
cmake_minimum_required(VERSION 3.12)

include(ExternalProject)

project(opentemple-native LANGUAGES CXX C)

#
# Set default properties
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Qt5Quick 6 REQUIRED)
find_package(Qt5QuickControls2 6 REQUIRED)
find_package(Qt5QmlWorkerScript 6 REQUIRED)

########### BEGIN FIXING MISSING QUICK PLUGINS
add_library(Qt5QuickPlugin STATIC IMPORTED)
set_target_properties(Qt5QuickPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/qtquick2plugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/qtquick2plugin.lib"
        )

add_library(Qt5QmlPlugin STATIC IMPORTED)
set_target_properties(Qt5QmlPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQml/qmlplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQml/qmlplugin.lib"
        )

add_library(QtQmlModelsPlugin STATIC IMPORTED)
set_target_properties(QtQmlModelsPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQml/Models/modelsplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQml/Models/modelsplugin.lib"
        )

add_library(QtQmlWorkerScriptPlugin STATIC IMPORTED)
set_target_properties(QtQmlWorkerScriptPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQml/WorkerScript/workerscriptplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQml/WorkerScript/workerscriptplugin.lib"
        )

add_library(QtGraphicalEffectsPlugin STATIC IMPORTED)
set_target_properties(QtGraphicalEffectsPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtGraphicalEffects/qtgraphicaleffectsplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtGraphicalEffects/qtgraphicaleffectsplugin.lib"
        )

add_library(QtGraphicalEffectsPrivatePlugin STATIC IMPORTED)
set_target_properties(QtGraphicalEffectsPrivatePlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtGraphicalEffects/private/qtgraphicaleffectsprivated.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtGraphicalEffects/private/qtgraphicaleffectsprivate.lib"
        )

add_library(QtQuick2WindowPlugin STATIC IMPORTED)
set_target_properties(QtQuick2WindowPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/Window/windowplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/Window/windowplugin.lib"
        )

target_link_libraries(Qt5::Quick INTERFACE Qt5QuickPlugin Qt5QmlPlugin QtQmlModelsPlugin QtQmlWorkerScriptPlugin QtGraphicalEffectsPlugin QtQuick2WindowPlugin QtGraphicalEffectsPrivatePlugin Qt5::QmlWorkerScript)
########### END FIXING MISSING QUICK PLUGINS

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
        ${CMAKE_CURRENT_LIST_DIR}/src/ui.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ui.h
        ${CMAKE_CURRENT_LIST_DIR}/src/GameViewItem.h
        ${CMAKE_CURRENT_LIST_DIR}/src/GameViewItem.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/GameViews.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/GameViews.h
        ${CMAKE_CURRENT_LIST_DIR}/src/cursor.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/rcc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/rcc_wrapper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/completion_source.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/completion_source.h
        src/dynamic_interop.cpp "src/qobject_interop.cpp" "src/qobject_interop.h")
if (WIN32)
    set(SOURCES ${SOURCES}
            ${CMAKE_CURRENT_LIST_DIR}/src/install_dir_finder_win.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/windows_wrapper.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/game_folders_windows.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/windows_jumplist.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/shell.cpp)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif ()

#
# Build  qmlnet
#
set(QMLNET_DIR ${CMAKE_CURRENT_LIST_DIR}/thirdparty/qmlnet/src/native/QmlNet)
FILE(GLOB_RECURSE QMLNET_H ${QMLNET_DIR}/QmlNet/*.h)
FILE(GLOB_RECURSE QMLNET_CPP ${QMLNET_DIR}/QmlNet/*.cpp)
set(SOURCES ${SOURCES}
        ${QMLNET_H}
        ${QMLNET_CPP}
        ${QMLNET_DIR}/QmlNet.cpp
        ${QMLNET_DIR}/QmlNet.h
        ${QMLNET_DIR}/QmlNetUtilities.cpp
        ${QMLNET_DIR}/QmlNetUtilities.h
)

add_library(opentemple SHARED ${SOURCES})
target_include_directories(opentemple PRIVATE ${QMLNET_DIR})
target_compile_definitions(opentemple PRIVATE QMLNET_NO_WIDGETS)
set_target_properties(opentemple PROPERTIES OUTPUT_NAME "OpenTemple.Native")
set_target_properties(opentemple PROPERTIES AUTOMOC TRUE
        AUTOMOC_EXECUTABLE "D:/qt/6/bin/moc.exe")
target_include_directories(opentemple PRIVATE ${CMAKE_CURRENT_LIST_DIR}/thirdparty/stb)
set_target_properties(opentemple PROPERTIES QT_PLUGINS_imageformats "Qt5::QICOPlugin")
set_target_properties(opentemple PROPERTIES QT_PLUGINS_qmltooling "-")
set_target_properties(opentemple PROPERTIES QT_PLUGINS_network "-")
target_link_libraries(opentemple libjpeg-turbo soloud Qt5::CorePrivate Qt5::Quick Qt5::QuickControls2 Qt5::Gui)
#set_property(TARGET opentemple APPEND PROPERTY QT_NO_PLUGINS Qt5::QICOPlugin)
target_include_directories(opentemple PRIVATE "D:/qt/6/include/QtANGLE")
