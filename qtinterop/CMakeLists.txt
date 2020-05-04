
add_library(qtinterop_obj OBJECT qobject_interop.cpp string_interop.cpp string_interop.h
        managed_delegate.cpp managed_delegate.h)
target_link_libraries(qtinterop_obj PRIVATE Qt::CorePrivate Qt::QmlPrivate Qt::Quick)
exclude_qt_plugins(qtinterop_obj)

add_library(qtinterop INTERFACE)
target_sources(qtinterop INTERFACE $<TARGET_OBJECTS:qtinterop_obj>)
target_include_directories(qtinterop INTERFACE ${CMAKE_CURRENT_LIST_DIR})
