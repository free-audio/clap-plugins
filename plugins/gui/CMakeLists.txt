# Dear ImGui is meant to be compiled within the project
set(IMGUI_DIR ${CLAP_PLUGINS_SOURCE_DIR}/imgui)
add_library(
  clap-plugins-imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp)
target_include_directories(clap-plugins-imgui PUBLIC ${IMGUI_DIR})
target_compile_definitions(clap-plugins-imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS
                                                     IMGUI_DEFINE_MATH_OPERATORS)
set_property(TARGET clap-plugins-imgui PROPERTY POSITION_INDEPENDENT_CODE true)

add_library(
  clap-plugins-gui STATIC
  abstract-gui-listener.hh
  abstract-gui-listener.cc
  imgui-gui.hh
  imgui-gui.cc
  platform-window.hh
  skins.hh
  skins.cc
  software-renderer.hh
  software-renderer.cc
  widgets.hh
  widgets.cc)
set_property(TARGET clap-plugins-gui PROPERTY CXX_STANDARD 20)
set_property(TARGET clap-plugins-gui PROPERTY POSITION_INDEPENDENT_CODE true)
target_link_libraries(clap-plugins-gui PUBLIC clap clap-plugins-imgui)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  include(${CLAP_PLUGINS_SOURCE_DIR}/cmake/wayland-protocols.cmake)
  find_package(PkgConfig REQUIRED)
  # 1.22: wl_surface.preferred_buffer_scale
  pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client>=1.22)
  pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
  target_sources(clap-plugins-gui PRIVATE wayland-floating.h wayland-window.hh wayland-window.cc
                                          x11-window.hh x11-window.cc)
  target_wayland_protocols(
    clap-plugins-gui
    stable/xdg-shell/xdg-shell.xml
    stable/viewporter/viewporter.xml
    staging/fractional-scale/fractional-scale-v1.xml
    staging/xdg-activation/xdg-activation-v1.xml
    staging/cursor-shape/cursor-shape-v1.xml
    # cursor-shape refers to the tablet tool interface
    unstable/tablet/tablet-unstable-v2.xml
    unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
    unstable/xdg-foreign/xdg-foreign-unstable-v2.xml)
  target_link_libraries(clap-plugins-gui PRIVATE PkgConfig::WAYLAND_CLIENT PkgConfig::XCB)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_sources(clap-plugins-gui PRIVATE win32-window.hh win32-window.cc)
  target_compile_definitions(clap-plugins-gui PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(clap-plugins-gui PRIVATE user32 gdi32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  enable_language(OBJCXX)
  target_sources(clap-plugins-gui PRIVATE cocoa-window.hh cocoa-window.mm)
  set_source_files_properties(cocoa-window.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_link_libraries(clap-plugins-gui PRIVATE "-framework Cocoa" "-framework CoreGraphics")
endif()
