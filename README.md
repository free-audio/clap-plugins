- [Example CLAP Plugins](#example-clap-plugins)
  - [The GUI](#the-gui)
  - [Notes on distributing a plugin](#notes-on-distributing-a-plugin)
- [Building](#building)
  - [Linux](#linux)
  - [macOS](#macos)
  - [Windows](#windows)
  - [Without GUI](#without-gui)
  - [Installing](#installing)
- [Trying the GUIs on Linux](#trying-the-guis-on-linux)

# Example CLAP Plugins

This repo serves as an example to demonstrate how to create a CLAP plugin.

<br/><a href="https://repology.org/project/clap-plugins/versions" target="_blank" rel="noopener" title="Packaging status"><img src="https://repology.org/badge/vertical-allrepos/clap-plugins.svg"></a>

## The GUI

The GUI is written with [Dear ImGui](https://github.com/ocornut/imgui), which is a submodule,
and rendered in software into a pixel buffer. It doesn't need any library besides the system ones,
nor any GPU context: each platform only has to show the pixels and to report the mouse events.

| Platform | Window API | Embedded | Floating |
|----------|------------|----------|----------|
| Linux    | X11        | yes      | yes      |
| Linux    | Wayland    | no       | yes, experimental, see [clap-wayland-experiment](https://github.com/free-audio/clap-wayland-experiment) |
| Windows  | Win32      | yes      | no       |
| macOS    | Cocoa      | yes      | no       |

Everything runs on the main thread. On Linux, the host's timer drives the GUI
([`clap.timer-support`](https://github.com/free-audio/clap/blob/main/include/clap/ext/timer-support.h));
on Windows and macOS, the window has its own timer.

The skins, one per plugin, are in [`skins.cc`](plugins/gui/skins.cc).
The plugins without a skin show a knob per parameter.

There is no keyboard input yet.

## Notes on distributing a plugin

A CLAP plugin should be self-contained: it should not rely upon symbols from the host, and it
should export only one symbol: `clap_entry`.

Even if you hide all your symbols, some may remain visible at unexpected places:
- Objective-C registers every class, including those coming from plugins, in a **flat namespace**.
  If two plugins define different classes with the same name, they clash, which results in
  undefined behavior. That's why our `NSView` subclass is created at runtime, with a name unique to
  the plugin's binary.
- Xlib's error handlers are process-wide, a plugin would replace the host's ones.
  That's why we use XCB on X11.
- Wayland objects belong to a connection: the plugin opens its own one rather than sharing the
  host's. The host and the plugin only exchange strings, the xdg-foreign handle and the
  xdg-activation token, with the extensions from
  [clap-wayland-experiment](https://github.com/free-audio/clap-wayland-experiment), a submodule.

# Building

You need CMake 3.17 or newer, a C++20 compiler, and the submodules:

```bash
git clone --recurse-submodules https://github.com/free-audio/clap-plugins
cd clap-plugins
```

The plugins are then in `builds/<preset>/plugins/Release/clap-plugins.clap`.

## Linux

The GUI needs `libwayland-client` 1.22 or newer, `wayland-protocols` 1.32 or newer, and `libxcb`,
for example Ubuntu 24.04 or newer.

```bash
# on ubuntu, adapt to your distribution and package manager
sudo apt install git ninja-build cmake pkg-config g++ libwayland-dev wayland-protocols libxcb1-dev

# on archlinux, adapt to your distribution and package manager
sudo pacman -S git ninja cmake pkgconf wayland wayland-protocols libxcb

cmake --preset ninja
cmake --build --preset ninja --config Release
```

## macOS

Install the Xcode command line tools, then:

```bash
brew install cmake ninja

cmake --preset ninja
cmake --build --preset ninja --config Release
```

## Windows

Install **Visual Studio 2022**, with its C++ and CMake components. Then in a PowerShell:

```ps1
cmake --preset vs
cmake --build --preset vs --config Release
```

## Without GUI

The `ninja-headless` and `vs-headless` presets build the plugins without GUI:

```bash
cmake --preset ninja-headless
cmake --build --preset ninja-headless --config Release
```

## Installing

Copy `clap-plugins.clap` where the hosts look for plugins, for example `~/.clap` on Linux,
`~/Library/Audio/Plug-Ins/CLAP` on macOS, or `%LOCALAPPDATA%\Programs\Common\CLAP` on Windows.

# Trying the GUIs on Linux

[`clap-gui-host`](tools/gui-host/main.cc) is a minimal host, which only shows a plugin's GUI.
It prints the parameter changes.

```bash
builds/ninja/tools/gui-host/Release/clap-gui-host \
  builds/ninja/plugins/Release/clap-plugins.clap com.github.free-audio.clap.synth
```

By default it uses the plugin's preferred window API. The options:
- `--x11` or `--wayland`: the window API
- `--floating`: a floating window rather than an embedded one; Wayland is always floating.
  Click in the host's window to show or hide the plugin's window.
- `--scale <factor>`: the scale given to the plugin with X11. By default, like a real host, it
  gives the desktop's scale: `Xft.dpi`, or with XWayland, the ratio between the X11 screen and
  the Wayland outputs' logical layout. The plugins don't guess it: they use the host's.

Without a plugin id, it shows the gain plugin. With an unknown one, it lists the plugins.
