set(CLAP_PLUGINS_REMOTE_GUI TRUE CACHE BOOL "Should the GUI be ran from another process?")
set(CLAP_PLUGINS_EMBED_QML TRUE CACHE BOOL "Embed QML resources into the plugin")
set(CLAP_PLUGINS_HEADLESS FALSE CACHE BOOL "Compile the plugins without a GUI")

if(CLAP_PLUGINS_HEADLESS)
  add_compile_definitions(CLAP_PLUGINS_HEADLESS)
endif()

if(CLAP_PLUGINS_EMBED_QML)
  add_compile_definitions(CLAP_PLUGINS_EMBED_QML)
endif()

if(NOT CLAP_PLUGINS_HEADLESS)
  add_subdirectory(io)
  add_subdirectory(gui)
endif()

add_library(
  clap-plugins MODULE

  audio-buffer.hh
  clap-entry.cc
  constants.hh
  container-of.hh
  context.hh
  core-plugin.cc
  core-plugin.hh
  domain-converter.hh
  fixed-point.hh
  intrusive-list.hh
  modulation-matrix.hh
  modules/adsr-module.cc
  modules/adsr-module.hh
  modules/module.cc
  modules/module.hh
  modules/root-module.cc
  modules/root-module.hh
  modules/voice-expander-module.cc
  modules/voice-expander-module.hh
  modules/voice-module.cc
  modules/voice-module.hh
  parameter.cc
  parameter.hh
  parameters.cc
  parameters.hh
  path-provider.cc
  path-provider.hh
  plugs/char-check/char-check.cc
  plugs/char-check/char-check.hh
  plugs/dc-offset/dc-offset.cc
  plugs/dc-offset/dc-offset.hh
  plugs/gain/gain.cc
  plugs/gain/gain.hh
  plugs/transport/transport-info.cc
  plugs/transport/transport-info.hh
  smoothed-value.hh
  stream-helper.hh
  value-types/decibel-value-type.hh
  value-types/decibel-value-type.cc
  value-types/simple-value-type.hh
  value-types/simple-value-type.cc
  value-types/value-type.cc
  value-types/value-type.hh
  )

if(NOT CLAP_PLUGINS_HEADLESS)
  if(CLAP_PLUGINS_REMOTE_GUI)
    target_compile_definitions(clap-plugins PUBLIC CLAP_REMOTE_GUI)
    target_link_libraries(clap-plugins PUBLIC clap-plugin-remote-gui)
    add_dependencies(clap-plugins clap-gui)
  else()
    target_compile_definitions(clap-plugins PUBLIC CLAP_LOCAL_GUI)
    target_link_libraries(clap-plugins PUBLIC clap-plugin-local-gui)
  endif()

  target_link_libraries(clap-plugins PUBLIC clap-io)
endif()

set_target_properties(clap-plugins PROPERTIES CXX_STANDARD 20)

target_link_libraries(clap-plugins PUBLIC clap-plugin-gui-common clap-helpers)

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

if(NOT CLAP_PLUGINS_HEADLESS AND NOT CLAP_PLUGINS_EMBED_QML)
  install(DIRECTORY gui/qml
    DESTINATION "lib/clap-plugins"
    PATTERN CMakeLists.txt EXCLUDE)
endif()
