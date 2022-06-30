- [Example Clap Plugins](#example-clap-plugins)
  - [Notes on GUI, static build vs dynamic build and symbols](#notes-on-gui-static-build-vs-dynamic-build-and-symbols)
- [Building on various platforms](#building-on-various-platforms)
  - [macOS, dynamic build with brew](#macos-dynamic-build-with-brew)
  - [macOS with vcpkg](#macos-with-vcpkg)
  - [Windows](#windows)
    - [Enable long path support](#enable-long-path-support)
    - [Build](#build)
  - [Linux, using system libraries (dynamic)](#linux-using-system-libraries-dynamic)
  - [Linux, using vcpkg (static)](#linux-using-vcpkg-static)

# Example Clap Plugins

This repo serves as an example to demonstrate how to create a CLAP plugin.

<br/><a href="https://repology.org/project/clap-plugins/versions" target="_blank" rel="noopener" title="Packaging status"><img src="https://repology.org/badge/vertical-allrepos/clap-plugins.svg"></a>

## Notes on GUI, static build vs dynamic build and symbols

The plugins use Qt for the GUI.

It is fine to dynamically link to Qt for a host, but it is very dangerous for a plugin.

Also one very important aspect of the plugin is the distribution.
Ideally a clap plugin should be self contained: it should not rely upon symbols from the host,
and it should export only one symbol: `clap_entry`.

You should be aware that even if you hide all your symbols some may still remain visible
at unexpected places. Objective-C seems to register every classes including those coming from
plugins in a **flat namespace**. Which means that if two plugins define two different
Objective-C classes but with the same, they will clash which will result in undeflined behavior.

Qt uses a few Objective-C classes on macOS. So it is crucial to use `QT_NAMESPACE`.

We have two different strategies to work with that.
1. **local**: statically link every thing
2. **remote**: start the gui in a child process

**1.** has the advantage of being simple to deploy.
**2.** is more complex due to its inter-process nature. It has a few advantages:
- if the GUI crash, the audio engine does not
- the GUI can use any libraries, won't be subject to symbol or library clash etc...

We abstracted the relation between the plugin and the GUI:
[`AbstractGui`](plugins/gui/abstract-gui.hh) and [`AbstractGuiListener`](plugins/gui/abstract-gui-listener.hh)
which lets us transparently insert proxies to support the **remote** model.

The GUI itself work with proxy objects to the parameters, transport info, ...
They are then bound into QML objects.
See [`Knob.qml`](plugins/gui/qml/clap/Knob.qml) and [`parameter-proxy.hh`](plugins/gui/parameter-proxy.hh).

We offer two options:
- static build, cmake preset: `ninja-vcpkg` or `vs-vcpkg` on Windows.
- dynamic builg, cmake preset: `ninja-system`

Static builds are convenient for deployment as they are self containded. They use the **local** gui model.

Dynamic builds will get your started quickly if your system provides Qt6,
and you have an host that do not expose the Qt symbols.
Static builds will require more time and space.

# Building on various platforms

## macOS, dynamic build with brew

```shell
# Install dependencies
brew install qt6 boost ninja cmake

# Checkout the code
git clone --recurse-submodules https://github.com/free-audio/clap-plugins
cd clap-plugins

# Build
cmake --preset ninja-system
cmake --build --preset ninja-system
```

## macOS with vcpkg

```shell
# Install build tools
brew install cmake ninja

# Checkout the code
git clone --recurse-submodules https://github.com/free-audio/clap-plugins
cd clap-plugins
scripts/build.sh
```

## Windows

### Enable long path support

Make sure your system supports long paths. Run this in an administrator PowerShell:

```powershell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```

Reference: https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=powershell

### Build

Install **Visual Studio 2022**; you can install it from the **Microsoft Store**. It can also install **git** and **CMake** for you.

Use the following command inside `Developer PowerShell For VS 2022`:
```powershell
# Checkout the code very close to the root to avoid windows long path issues...
cd c:\
git clone --recurse-submodules https://github.com/free-audio/clap-plugins c-p
cd c-p

scripts/build.sh
```

## Linux, using system libraries (dynamic)

```bash
# on archlinux, adapt to your distribution and package manager
sudo pacman -S qt boost git ninja cmake

git clone --recurse-submodules https://github.com/free-audio/clap-plugins
cd clap-plugins
cmake --preset ninja-system
cmake --build --preset ninja-system
```

## Linux, using vcpkg (static)

```bash
git clone --recurse-submodules https://github.com/free-audio/clap-plugins
cd clap-plugins
scripts/build.sh
```
