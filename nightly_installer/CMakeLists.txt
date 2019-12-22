
#
# Set default properties
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp app.manifest resource.h app.rc certificate.h os.h)

add_executable(InstallOpenTempleNightly WIN32 ${SOURCES})
target_link_libraries(InstallOpenTempleNightly PRIVATE comctl32.lib Crypt32.lib Cryptui.lib)
set_property(TARGET InstallOpenTempleNightly PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(InstallOpenTempleNightly PRIVATE UNICODE _UNICODE)
