
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

## Dependencies

[SoLoud](https://github.com/jarikomppa/soloud) is used for outputting sound on various platforms.
*License: Zlib/LibPng*

[libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo) is used to decode JPEG images much faster than standard JPEG decoders. 
Since the background art for all maps in ToEE is stored in JPEG images, this has great performance benefits for us. 
*License: BSD*

