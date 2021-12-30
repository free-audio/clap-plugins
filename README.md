# Minimal Clap Host

This repo serves as an example to demonstrate how to create a CLAP host and plugins.

## Building on various platforms

### macOS

```shell
# Install dependencies
brew install qt6 pkgconfig rtaudio rtmidi

# Checkout the code
git clone --recurse-submodules https://github.com/free-audio/clap-examples
cd clap-plugins

# Build
cmake --preset xcode-system
cmake --build builds/xcode-system --config Release
```

### Windows

#### Enable long path support

Make sure your system supports long paths. Run this in an administrator PowerShell:

```powershell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```

Reference: https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=powershell

#### Build

Install **Visual Studio 2022**; you can install it from the **Microsoft Store**. It can also install **git** and **CMake** for you.

Use the following command inside `Developer PowerShell For VS 2022`:
```powershell
# Checkout the code very close to the root to avoid windows long path issues...
cd c:\
git clone --recurse-submodules https://github.com/free-audio/clap-examples clap
cd clap

# Build
cmake --preset vs-vcpkg
cmake --build builds/vs-vcpkg --config Release

# Build installer
cmake --build builds/vs-vcpkg --target PACKAGE --config Release
```
