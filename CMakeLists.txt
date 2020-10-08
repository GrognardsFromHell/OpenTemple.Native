
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

find_package(Qt5 5.15 COMPONENTS Core Gui Qml Quick QuickControls2 QmlWorkerScript QuickShapes REQUIRED)

#target_link_libraries(Qt::Quick INTERFACE Qt5QuickPlugin Qt5QmlPlugin QtQmlModelsPlugin QtQmlWorkerScriptPlugin QtGraphicalEffectsPlugin QtQuick2WindowPlugin QtGraphicalEffectsPrivatePlugin Qt::QmlWorkerScript QtQuickControls2Plugin QtQuickLayoutsPlugin QtQuickTemplates2Plugin)
########### END FIXING MISSING QUICK PLUGINS

include(cmake/qtplugins.cmake)
include(cmake/soloud.cmake)
include(cmake/libjpegturbo.cmake)
include(cmake/qmlnet.cmake)
add_subdirectory(qmlintrospection)
add_subdirectory(qtinterop)
add_subdirectory(game)
add_subdirectory(thirdparty_wrappers)
add_subdirectory(ffmpeg_bink)

add_library(opentemple SHARED thirdparty_wrappers/soloud.def)
target_link_libraries(opentemple PRIVATE game qtinterop qmlintrospection thirdparty_wrappers qmlnet)

qt_import_plugins(opentemple
        EXCLUDE_BY_TYPE imageformats
        INCLUDE Qt::QICOPlugin
        INCLUDE Qt::QTgaPlugin
        EXCLUDE_BY_TYPE iconengines
        EXCLUDE_BY_TYPE qmltooling
        EXCLUDE_BY_TYPE generic
        EXCLUDE_BY_TYPE bearer
        )
target_link_libraries(opentemple PRIVATE Qt::CorePrivate Qt::QmlPrivate Qt::GuiPrivate Qt5QuickPlugin Qt5QmlPlugin Qt::QuickControls2 QtQmlModelsPlugin QtQmlWorkerScriptPlugin QtGraphicalEffectsPlugin QtQuick2WindowPlugin QtGraphicalEffectsPrivatePlugin Qt::QmlWorkerScript QtQuickControls2Plugin QtQuickLayoutsPlugin QtQuickTemplates2Plugin QmlShapesPlugin Qt::QuickShapes)

target_include_directories(opentemple PRIVATE ${QMLNET_DIR})
target_compile_definitions(opentemple PRIVATE QMLNET_NO_WIDGETS)
set_target_properties(opentemple PROPERTIES OUTPUT_NAME "OpenTemple.Native")

add_subdirectory(managed)

