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
  clap-plugin-imgui-gui STATIC
  imgui-gui.hh
  imgui-gui.cc
  imgui-gui-factory.hh
  imgui-gui-factory.cc
  platform-window.hh
  skins.hh
  skins.cc
  software-renderer.hh
  software-renderer.cc
  widgets.hh
  widgets.cc)
set_property(TARGET clap-plugin-imgui-gui PROPERTY CXX_STANDARD 20)
set_property(TARGET clap-plugin-imgui-gui PROPERTY POSITION_INDEPENDENT_CODE true)
target_link_libraries(clap-plugin-imgui-gui PUBLIC clap-plugin-gui-common clap-plugins-imgui)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  include(${CLAP_PLUGINS_SOURCE_DIR}/cmake/wayland-protocols.cmake)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
  target_sources(clap-plugin-imgui-gui PRIVATE wayland-window.hh wayland-window.cc)
  target_wayland_protocols(
    clap-plugin-imgui-gui
    stable/xdg-shell/xdg-shell.xml
    stable/viewporter/viewporter.xml
    staging/fractional-scale/fractional-scale-v1.xml
    staging/xdg-activation/xdg-activation-v1.xml
    staging/cursor-shape/cursor-shape-v1.xml
    # cursor-shape refers to the tablet tool interface
    unstable/tablet/tablet-unstable-v2.xml
    unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
    unstable/xdg-foreign/xdg-foreign-unstable-v2.xml)
  target_link_libraries(clap-plugin-imgui-gui PRIVATE PkgConfig::WAYLAND_CLIENT)
endif()
