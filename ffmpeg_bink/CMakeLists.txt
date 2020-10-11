
#
# Download ffmpeg pre-built since it takes 10 minutes and requires special setup to build.
#
set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/thirdparty/ffmpeg-bink/")

if(NOT EXISTS ${FFMPEG_DIR})
    message(FATAL_ERROR "Please provide the precompiled ffmpeg-library in ${FFMPEG_DIR}. See https://github.com/GrognardsFromHell/ffmpeg-bink/releases")
endif()

# Build a shared library to encapsulate all of FFMPEG's code
# Since we have to hack together a build with LTCG using /FORCE
# since FFMPEG uses the assumption that code in unreachable branches
# can safely reference undefined functions. In LTCG mode however,
# MSVC will complain about this.
add_library(
        ffmpeg-bink
        SHARED
        videoplayer.cpp
        videoplayer.h)
target_link_directories(ffmpeg-bink PUBLIC ${FFMPEG_DIR}lib/)
target_link_libraries(ffmpeg-bink PUBLIC libavcodec libavformat libavutil)
target_include_directories(ffmpeg-bink PRIVATE ${FFMPEG_DIR}include)
target_link_libraries(ffmpeg-bink PRIVATE bcrypt.lib)
target_compile_definitions(ffmpeg-bink PRIVATE -DBUILDING_VIDEOPLAYER)
target_link_options(ffmpeg-bink PRIVATE /force)
set_target_properties(ffmpeg-bink PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

#
# QtQuick Item for video playback
#
add_library(
        bink-video-item
        STATIC
        BinkVideoItem.cpp BinkVideoItem.h VideoMaterial.cpp VideoMaterial.h VideoFrame.h VideoAudioSource.cpp VideoAudioSource.h)
target_link_libraries(bink-video-item PUBLIC ffmpeg-bink soloud)
target_include_directories(bink-video-item INTERFACE ${CMAKE_CURRENT_LIST_DIR})
