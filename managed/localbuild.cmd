dotnet msbuild -t:Clean,Build,Pack,InstallLocal Interop
dotnet restore BuildTask
dotnet msbuild -t:Clean,Build,Pack,InstallLocal BuildTask
