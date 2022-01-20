set(CLAP_PLUGIN_GUI_MODEL "" CACHE STRING "remote, local")

if(CLAP_PLUGIN_GUI_MODEL STREQUAL "local")
  set(use_local_gui TRUE)
  set(use_remote_gui FALSE)
elseif(CLAP_PLUGIN_GUI_MODEL STREQUAL "remote")
  set(use_local_gui FALSE)
  set(use_remote_gui TRUE)
endif()

add_subdirectory(io)
add_subdirectory(gui)

add_library(
  clap-plugins SHARED
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

if(use_local_gui)
  target_compile_definitions(clap-plugins PUBLIC CLAP_LOCAL_GUI)
  target_link_libraries(clap-plugins PUBLIC clap-plugin-local-gui)
elseif(use_remote_gui)
  target_compile_definitions(clap-plugins PUBLIC CLAP_REMOTE_GUI)
  target_link_libraries(clap-plugins PUBLIC clap-plugin-remote-gui)
  add_dependencies(clap-plugins clap-gui)
endif()

set_target_properties(clap-plugins PROPERTIES CXX_STANDARD 20 SUFFIX ".clap" PREFIX "")

target_link_libraries(clap-plugins PUBLIC clap-helpers clap-io Boost::serialization Boost::iostreams)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(clap-plugins PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-clap-plugins.version)
  target_link_libraries(clap-plugins PRIVATE -Wl,-z,defs)
endif()

install(TARGETS clap-plugins DESTINATION "lib/clap" COMPONENT plugins)
install(DIRECTORY gui/qml DESTINATION "lib/clap-examples")