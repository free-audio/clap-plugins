# Minimal Clap Host and Plugins

This repo serves as an example to demonstrate how to create a CLAP host and plugins.

The plugins are under heavy refactoring and are not working yet.

## Building on various platforms

### macOS with brew

This options is the quickest to play with the examples, but it wont let you run the
example host and plugin together, for that see the vcpkg option.

Choose this option if you want to test your plugin with clap-host or if you want to
test the clap-plugins with your host.

Note that the resulting build should not be distributed.

```shell
# Install dependencies
brew install boost qt6 pkgconfig rtaudio rtmidi ninja cmake

# Checkout the code
git clone --recurse-submodules https://github.com/free-audio/clap-examples
cd clap-examples

# Build
cmake --preset ninja-system -DCLAP_PLUGIN_GUI_MODEL=local
cmake --build builds/ninja-system --config Release
```

### macOS with vcpkg

This option takes the longuest to build as it requires to build Qt twice.
Even if Qt is built statically and all symbols are hidden, there will still
be a symbol clash due to objective-c's runtime which registers all classes
into a flat namespace. For that we must rely upon `QT_NAMESPACE` which puts
every symbols from Qt in the namespace specified by `QT_NAMESPACE`.

Wait for this [PR](https://github.com/microsoft/vcpkg/pull/22713) to complete before trying it.

```shell
# Install build tools
brew install cmake ninja

# Checkout the code
git clone --recurse-submodules https://github.com/free-audio/clap-examples
cd clap-examples

# Build the host
cmake --preset macos-arm64-host
cmake --build --preset macos-arm64-host --config Release

# Build the plugins
cmake --preset macos-arm64-plugins
cmake --build --preset macos-arm64-plugins --config Release
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
