include(${CLAP_PLUGINS_SOURCE_DIR}/cmake/wayland-protocols.cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client>=1.22)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)

add_executable(clap-gui-host main.cc)
set_property(TARGET clap-gui-host PROPERTY CXX_STANDARD 20)
target_wayland_protocols(
  clap-gui-host
  stable/xdg-shell/xdg-shell.xml
  staging/xdg-activation/xdg-activation-v1.xml
  staging/cursor-shape/cursor-shape-v1.xml
  unstable/tablet/tablet-unstable-v2.xml
  unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
  unstable/xdg-foreign/xdg-foreign-unstable-v2.xml)
target_include_directories(clap-gui-host PRIVATE ${CLAP_PLUGINS_SOURCE_DIR}/plugins/gui)
target_link_libraries(clap-gui-host PRIVATE clap PkgConfig::WAYLAND_CLIENT PkgConfig::XCB ${CMAKE_DL_LIBS})
