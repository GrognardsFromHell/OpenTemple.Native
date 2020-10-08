
function (exclude_qt_plugins)
    qt_import_plugins(${ARGV0}
            EXCLUDE_BY_TYPE imageformats
            EXCLUDE_BY_TYPE platforms
            EXCLUDE_BY_TYPE iconengines
            EXCLUDE_BY_TYPE qmltooling
            EXCLUDE_BY_TYPE generic
            EXCLUDE_BY_TYPE bearer
            )
endfunction()


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

add_library(QmlShapesPlugin STATIC IMPORTED)
set_target_properties(QmlShapesPlugin PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG "${_qt5Quick_install_prefix}/qml/QtQuick/Shapes/qmlshapesplugind.lib"
        IMPORTED_LOCATION "${_qt5Quick_install_prefix}/qml/QtQuick/Shapes/qmlshapesplugin.lib"
        )
