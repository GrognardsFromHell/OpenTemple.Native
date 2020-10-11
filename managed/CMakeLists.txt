
ExternalProject_Add(
        managed-interop
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/Interop
        CONFIGURE_COMMAND ""
        BUILD_COMMAND dotnet build
        BUILD_IN_SOURCE true
        INSTALL_COMMAND dotnet msbuild -t:Pack,InstallLocal
        TEST_COMMAND dotnet test
        TEST_AFTER_INSTALL true
        DEPENDS opentemple
)
