set(CLAP_PLUGINS_REMOTE_GUI TRUE CACHE BOOL "Should the GUI be ran from another process?")
set(CLAP_PLUGINS_EMBED_QML TRUE CACHE BOOL "Embed QML resources into the plugin")

if(CLAP_PLUGINS_EMBED_QML)
  add_compile_definitions(CLAP_PLUGINS_EMBED_QML)
endif()

add_subdirectory(io)
add_subdirectory(gui)

add_library(
  clap-plugins MODULE
  clap-entry.cc
  parameters.cc
  parameters.hh
  parameter.hh
  parameter-interpolator.hh
  core-plugin.hh
  core-plugin.cc
  stream-helper.hh

  path-provider.hh
  path-provider.cc

  plugs/dc-offset/dc-offset.hh
  plugs/dc-offset/dc-offset.cc
  plugs/gain/gain.hh
  plugs/gain/gain.cc
  plugs/transport/transport-info.hh
  plugs/transport/transport-info.cc)

if(CLAP_PLUGINS_REMOTE_GUI)
  target_compile_definitions(clap-plugins PUBLIC CLAP_REMOTE_GUI)
  target_link_libraries(clap-plugins PUBLIC clap-plugin-remote-gui)
  add_dependencies(clap-plugins clap-gui)
else()
  target_compile_definitions(clap-plugins PUBLIC CLAP_LOCAL_GUI)
  target_link_libraries(clap-plugins PUBLIC clap-plugin-local-gui)
endif()

set_target_properties(clap-plugins PROPERTIES CXX_STANDARD 20)

target_link_libraries(clap-plugins PUBLIC clap-plugin-gui-common clap-helpers clap-io)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(clap-plugins PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-clap-plugins.version)
  target_link_libraries(clap-plugins PRIVATE -Wl,-z,defs)
  set_target_properties(clap-plugins PROPERTIES SUFFIX ".clap" PREFIX "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_options(clap-plugins PRIVATE -exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/macos-symbols.txt)
  #target_link_libraries(clap-plugins PRIVATE -Wl,-z,defs)

  set_target_properties(clap-plugins PROPERTIES
            BUNDLE True
            BUNDLE_EXTENSION clap
            MACOSX_BUNDLE_GUI_IDENTIFIER org.clap.example-plugins
            MACOSX_BUNDLE_BUNDLE_NAME clap-example-plugins
            MACOSX_BUNDLE_BUNDLE_VERSION "1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/plugins.plist.in
            )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_target_properties(clap-plugins PROPERTIES SUFFIX ".clap" PREFIX "")
endif()

install(TARGETS clap-plugins DESTINATION "lib/clap" COMPONENT plugins)
install(DIRECTORY gui/qml DESTINATION "lib/clap-examples")
