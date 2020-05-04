
#
# Build  qmlnet
#
set(QMLNET_DIR ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/qmlnet/src/native/QmlNet)
FILE(GLOB_RECURSE QMLNET_H ${QMLNET_DIR}/QmlNet/*.h)
FILE(GLOB_RECURSE QMLNET_CPP ${QMLNET_DIR}/QmlNet/*.cpp)

add_library(qmlnet_obj OBJECT ${QMLNET_H}
        ${QMLNET_CPP}
        ${QMLNET_DIR}/QmlNet.cpp
        ${QMLNET_DIR}/QmlNet.h
        ${QMLNET_DIR}/QmlNetUtilities.cpp
        ${QMLNET_DIR}/QmlNetUtilities.h)
target_compile_definitions(qmlnet_obj PRIVATE QMLNET_NO_WIDGETS)
target_include_directories(qmlnet_obj PRIVATE ${QMLNET_DIR})
set(qmlnet_libs Qt::CorePrivate Qt::Gui Qt::Qml Qt::QuickControls2)
set_target_properties(qmlnet_obj PROPERTIES AUTOMOC TRUE)
target_link_libraries(qmlnet_obj PRIVATE ${qmlnet_libs})
exclude_qt_plugins(qmlnet_obj)

add_library(qmlnet INTERFACE)
target_sources(qmlnet INTERFACE $<TARGET_OBJECTS:qmlnet_obj>)
target_include_directories(qmlnet INTERFACE ${QMLNET_DIR})
target_link_libraries(qmlnet INTERFACE ${qmlnet_libs})
