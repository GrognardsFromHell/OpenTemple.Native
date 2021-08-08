
file(GLOB GAME_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        *.cpp *.h
        interop/*.cpp interop/*.h
        logging/*.cpp logging/*.h
        )

add_library(game_obj OBJECT ${GAME_SOURCES})
if (WIN32)
    file(GLOB_RECURSE GAME_WIN32_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS win32/*.cpp win32/*.h rendering/*.cpp rendering/*.h)
    target_sources(game_obj PRIVATE ${GAME_WIN32_SOURCES})
    target_compile_definitions(game_obj PRIVATE -D_CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(game_obj PRIVATE -DUNICODE)
    target_compile_definitions(game_obj PRIVATE -D_UNICODE)
    target_link_libraries(game_obj soloud)
endif ()

add_library(game INTERFACE)
target_sources(game INTERFACE $<TARGET_OBJECTS:game_obj>)
target_link_libraries(game INTERFACE RuntimeObject.lib dwrite.lib)
