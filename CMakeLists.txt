
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

find_package(Qt5 5.15 COMPONENTS Core Gui Qml Quick QuickControls2 QmlWorkerScript REQUIRED)

########### BEGIN FIXING MISSING QUICK PLUGINS
add_library(Qt5QuickPlugin STATIC IMPORTED)
set_target_properties(Qt5QuickPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick.2/qtquick2plugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick.2/qtquick2plugin.lib"
        )

add_library(QtQuickControls2Plugin STATIC IMPORTED)
set_target_properties(QtQuickControls2Plugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/Controls.2/qtquickcontrols2plugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/Controls.2/qtquickcontrols2plugin.lib"
        )

add_library(QtQuickLayoutsPlugin STATIC IMPORTED)
set_target_properties(QtQuickLayoutsPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/Layouts/qquicklayoutsplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/Layouts/qquicklayoutsplugin.lib"
        )

add_library(QtQuickTemplates2Plugin STATIC IMPORTED)
set_target_properties(QtQuickTemplates2Plugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/Templates.2/qtquicktemplates2plugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/Templates.2/qtquicktemplates2plugin.lib"
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
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQml/Models.2/modelsplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQml/Models.2/modelsplugin.lib"
        )

add_library(QtQmlWorkerScriptPlugin STATIC IMPORTED)
set_target_properties(QtQmlWorkerScriptPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQml/WorkerScript.2/workerscriptplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQml/WorkerScript.2/workerscriptplugin.lib"
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
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/Window.2/windowplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/Window.2/windowplugin.lib"
        )

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
target_link_libraries(opentemple PRIVATE Qt::CorePrivate Qt::QmlPrivate Qt::GuiPrivate Qt5QuickPlugin Qt5QmlPlugin Qt::QuickControls2 QtQmlModelsPlugin QtQmlWorkerScriptPlugin QtGraphicalEffectsPlugin QtQuick2WindowPlugin QtGraphicalEffectsPrivatePlugin Qt::QmlWorkerScript QtQuickControls2Plugin QtQuickLayoutsPlugin QtQuickTemplates2Plugin)

target_include_directories(opentemple PRIVATE ${QMLNET_DIR})
target_compile_definitions(opentemple PRIVATE QMLNET_NO_WIDGETS)
set_target_properties(opentemple PROPERTIES OUTPUT_NAME "OpenTemple.Native")

add_subdirectory(managed)

