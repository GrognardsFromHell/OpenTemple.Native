
# Native Code for OpenTemple

This repository contains the native code required for OpenTemple to run. It's packaged as a NuGet package and
distributed using GitHub packages for various platforms. This is done to reduce the ramp-up time for developers
on OpenTemple by removing the requirement to install the various dependencies required to compile the native
code.

## Building

<img src="https://github.com/GrognardsFromHell/OpenTemple.Native/workflows/CI/badge.svg" alt="CI Status">

We recommend looking at how GitHub Actions [builds the project](.github/workflows/ci.yml).

### Manual Building

[CMake](https://cmake.org/) 3.12 or later is required. In general, standard CMake practices apply.

[Nasm](https://www.nasm.us/), which is used to build libjpeg-turbo is included in this repository in the `tools-win`
folder to make compiling easier for Windows users.

Visual Studio 2019 is required to build this project. We recommend the free Community version (or the standalone build tools).

### Using NuGet Package Locally

Sometimes you may want to use the NuGet package built by this repository in your local OpenTemple build directly.
To do this, build the native component using CMake as described above, and then run `localbuild.cmd` from the
managed subdirectory. This uses the build output from the `cmake-build-debug` directory. You need to use this
as your CMake build directory, otherwise the native DLL will not be included in the NuGet package properly.
Once you have run the script, change the OpenTemple.Interop dependency in your OpenTemple Core project
file to the version number printed by `localbuild.cmd` to make use of the local package.  

## Dependencies

[SoLoud](https://github.com/jarikomppa/soloud) is used for outputting sound on various platforms.
*License: Zlib/LibPng*

[libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo) is used to decode JPEG images much faster than standard JPEG decoders. 
Since the background art for all maps in ToEE is stored in JPEG images, this has great performance benefits for us. 
*License: BSD*

[Modern C++ Wrappers Around Windows Registry C API](https://github.com/GiovanniDicanio/WinReg) is used to lessen the burden of accessing the Windows Registry.
*License: MIT*

[stb_image](https://github.com/nothings/stb) is used for certain image decoding tasks (primarily TGA). 
*License: public domain*
