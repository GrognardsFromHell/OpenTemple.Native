
add_library(thirdparty_wrappers_obj OBJECT stb_image_wrapper.cpp libjpeg_turbo_wrapper.cpp)
target_include_directories(thirdparty_wrappers_obj PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/stb)

target_link_libraries(thirdparty_wrappers_obj PUBLIC libjpeg-turbo soloud)

add_library(thirdparty_wrappers INTERFACE)
target_sources(thirdparty_wrappers INTERFACE $<TARGET_OBJECTS:thirdparty_wrappers_obj>)
target_link_libraries(thirdparty_wrappers INTERFACE libjpeg-turbo soloud)
