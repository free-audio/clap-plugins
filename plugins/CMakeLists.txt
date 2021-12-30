find_package(Boost REQUIRED COMPONENTS serialization iostreams)

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

  abstract-gui.hh
  remote-gui.hh
  remote-gui.cc

  plugs/dc-offset/dc-offset.hh
  plugs/dc-offset/dc-offset.cc
  plugs/gain/gain.hh
  plugs/gain/gain.cc
  plugs/transport/transport-info.hh
  plugs/transport/transport-info.cc)

set_target_properties(clap-plugins PROPERTIES CXX_STANDARD 20 SUFFIX ".clap" PREFIX "")

target_link_libraries(clap-plugins clap-helpers clap-io Boost::serialization Boost::iostreams)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(clap-plugins -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-clap-plugins.version)
  target_link_libraries(clap-plugins -Wl,-z,defs)
endif()

install(TARGETS clap-plugins DESTINATION "lib/clap")
install(DIRECTORY qml DESTINATION "lib/clap-examples")