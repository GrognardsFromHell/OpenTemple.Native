
add_library(qmlintrospection_obj OBJECT qmlinfo.cpp qmlinfo.h typeinfo.h propinfo.h
        typelibrary.cpp typelibrary.h
        typeref.h methodinfo.h c_interface.cpp enuminfo.h)
target_link_libraries(qmlintrospection_obj PRIVATE Qt::Gui Qt::QmlPrivate qtinterop)
exclude_qt_plugins(qmlintrospection_obj)

add_library(qmlintrospection INTERFACE)
target_sources(qmlintrospection INTERFACE $<TARGET_OBJECTS:qmlintrospection_obj>)

