set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# GUI interfaces
add_library(
  clap-plugin-gui-common
  abstract-gui.hh abstract-gui.cc abstract-gui-listener.hh
  abstract-gui-listener.cc abstract-gui-factory.hh abstract-gui-factory.cc
  gui-handle.hh gui-handle.cc)
set_property(TARGET clap-plugin-gui-common PROPERTY CXX_STANDARD 20)
set_property(TARGET clap-plugin-gui-common PROPERTY POSITION_INDEPENDENT_CODE
                                                    true)
target_link_libraries(clap-plugin-gui-common PUBLIC clap)

# Actual GUI implementation and display
add_library(
  clap-plugin-gui
  gui.hh
  gui.cc
  parameter-proxy.hh
  parameter-proxy.cc
  plugin-proxy.hh
  plugin-proxy.cc
  track-info-proxy.hh
  track-info-proxy.cc
  transport-proxy.hh
  transport-proxy.cc
  undo-proxy.hh
  undo-proxy.cc)
set_property(TARGET clap-plugin-gui PROPERTY CXX_STANDARD 20)
target_link_libraries(clap-plugin-gui PUBLIC clap-io clap-plugin-gui-common)
target_link_libraries(clap-plugin-gui PUBLIC Qt6::Quick)
set_property(TARGET clap-plugin-gui PROPERTY POSITION_INDEPENDENT_CODE true)
qt6_import_qml_plugins(clap-plugin-gui)

if(CLAP_PLUGINS_EMBED_QML)
  add_subdirectory(qml)
  target_link_libraries(clap-plugin-gui PUBLIC clap-qml-skins clap-qml-lib clap-qml-libplugin)
endif()

# Code for having the GUI with the plugin
add_library(
  clap-plugin-local-gui
  local-gui-factory.hh
  local-gui-factory.cc
  threaded-gui-factory.hh
  threaded-gui-factory.cc
  threaded-gui-proxy.hh
  threaded-gui-proxy.cc
  timer.hh
  timer.cc
  cf-timer.hh
  cf-timer.cc
  win32-timer.hh
  win32-timer.cc)
set_property(TARGET clap-plugin-local-gui PROPERTY CXX_STANDARD 20)
set_property(TARGET clap-plugin-local-gui PROPERTY POSITION_INDEPENDENT_CODE true)
target_link_libraries(clap-plugin-local-gui PUBLIC clap-plugin-gui)
