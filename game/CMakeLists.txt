
add_library(game_obj OBJECT
        vector_wrapper.cpp
        ui.cpp
        ui.h
        GameViewItem.h
        GameViewItem.cpp
        GameViews.cpp
        GameViews.h
        cursor.cpp
        completion_source.cpp
        completion_source.h
        utils.h
        dynamic_interop.cpp
        static_plugins.cpp
        qml_registration.cpp)
if (WIN32)
    target_sources(game_obj PRIVATE
            install_dir_finder_win.cpp
            game_folders_windows.cpp
            windows_jumplist.cpp
            shell.cpp)
    target_compile_definitions(game_obj PRIVATE -D_CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(game_obj PRIVATE -DUNICODE)
    target_compile_definitions(game_obj PRIVATE -D_UNICODE)
endif ()
target_link_libraries(game_obj PRIVATE Qt::Quick Qt::GuiPrivate Qt5::Gui_EGL Qt5::Gui_GLESv2 qtinterop bink-video-item)
exclude_qt_plugins(game_obj)
set_target_properties(game_obj PROPERTIES AUTOMOC TRUE)

add_library(game INTERFACE)
target_sources(game INTERFACE $<TARGET_OBJECTS:game_obj>)
target_link_libraries(game INTERFACE Qt::Quick Qt::GuiPrivate Qt5::Gui_EGL Qt5::Gui_GLESv2 qtinterop bink-video-item)
