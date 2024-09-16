set(CLAP_PLUGINS_REMOTE_GUI TRUE CACHE BOOL "Should the GUI be ran from another process?")
set(CLAP_PLUGINS_EMBED_QML TRUE CACHE BOOL "Embed QML resources into the plugin")
set(CLAP_PLUGINS_HEADLESS FALSE CACHE BOOL "Compile the plugins without a GUI")

if(CLAP_PLUGINS_HEADLESS)
  add_compile_definitions(CLAP_PLUGINS_HEADLESS)
else()
  find_package(Qt6 COMPONENTS Gui Qml Quick REQUIRED)
  add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)

  if(CLAP_PLUGINS_EMBED_QML)
    add_compile_definitions(CLAP_PLUGINS_EMBED_QML)
  endif()

  add_subdirectory(io)
  add_subdirectory(gui)
endif()

add_library(
  clap-plugins-core STATIC

  audio-buffer.cc
  audio-buffer.hh
  constants.hh
  container-of.hh
  context.hh
  core-plugin.cc
  core-plugin.hh
  domain-converter.hh
  fixed-point.hh
  intrusive-list.hh
  merge-process-status.hh
  merge-process-status.cc
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
  modules/svf-module.hh
  modules/svf-module.cc
  modules/digi-osc-module.hh
  modules/digi-osc-module.cc
  parameter.cc
  parameter.hh
  parameters.cc
  parameters.hh
  path-provider.cc
  path-provider.hh
  plugs/adsr/adsr-plug.cc
  plugs/adsr/adsr-plug.hh
  plugs/char-check/char-check.cc
  plugs/char-check/char-check.hh
  plugs/dc-offset/dc-offset.cc
  plugs/dc-offset/dc-offset.hh
  plugs/gain/gain.cc
  plugs/gain/gain.hh
  plugs/latency/latency.cc
  plugs/latency/latency.hh
  plugs/offline-latency/offline-latency.hh
  plugs/offline-latency/offline-latency.cc
  plugs/realtime-requirement/realtime-requirement.hh
  plugs/realtime-requirement/realtime-requirement.cc
  plugs/synth/synth.hh
  plugs/synth/synth.cc
  plugs/track-info/track-info.cc
  plugs/track-info/track-info.hh
  plugs/transport/transport-info.cc
  plugs/transport/transport-info.hh
  plugs/svf/svf-plug.hh
  plugs/svf/svf-plug.cc
  plugs/undo-test/undo-test.hh
  plugs/undo-test/undo-test.cc
  smoothed-value.hh
  sample-delay.cc
  sample-delay.hh
  stream-helper.cc
  stream-helper.hh
  tuning-provider.cc
  tuning-provider.hh
  value-types/decibel-value-type.hh
  value-types/decibel-value-type.cc
  value-types/simple-value-type.hh
  value-types/simple-value-type.cc
  value-types/value-type.cc
  value-types/value-type.hh
  value-types/frequency-value-type.hh
  value-types/frequency-value-type.cc
  value-types/enumerated-value-type.hh
  value-types/enumerated-value-type.cc
  )
target_compile_definitions(clap-plugins-core PUBLIC _USE_MATH_DEFINES)
set_target_properties(clap-plugins-core PROPERTIES CXX_STANDARD 20)
set_target_properties(clap-plugins-core PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

if(NOT CLAP_PLUGINS_HEADLESS)
  if(CLAP_PLUGINS_REMOTE_GUI)
    target_compile_definitions(clap-plugins-core PUBLIC CLAP_REMOTE_GUI)
    target_link_libraries(clap-plugins-core PUBLIC clap-plugin-remote-gui)
    add_dependencies(clap-plugins-core clap-gui)
  else()
    target_compile_definitions(clap-plugins-core PUBLIC CLAP_LOCAL_GUI)
    target_link_libraries(clap-plugins-core PUBLIC clap-plugin-local-gui)
  endif()

  target_link_libraries(clap-plugins-core PUBLIC clap-io)
endif()

if (NOT CLAP_PLUGINS_HEADLESS)
  target_link_libraries(clap-plugins-core PUBLIC clap-plugin-gui-common)
endif()
target_link_libraries(clap-plugins-core PUBLIC clap-helpers)

add_library(clap-plugins MODULE clap-entry.cc)
target_link_libraries(clap-plugins PUBLIC clap-plugins-core)
set_target_properties(clap-plugins PROPERTIES CXX_STANDARD 20)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(clap-plugins PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-clap-plugins.version)
  target_link_libraries(clap-plugins PRIVATE -Wl,-z,defs)
  set_target_properties(clap-plugins PROPERTIES SUFFIX ".clap" PREFIX "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_options(clap-plugins PRIVATE -exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/macos-symbols.txt)

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

# CLAP_BUILD_TESTS is inherited from clap
if (${CLAP_BUILD_TESTS})
    enable_testing()
    find_package(Catch2 3 QUIET)

    if (Catch2_FOUND)
      add_executable(clap-plugins-tests EXCLUDE_FROM_ALL
          tests/value-type-tests.cc)
      set_target_properties(clap-plugins-tests PROPERTIES CXX_STANDARD 20)
      target_link_libraries(clap-plugins-tests clap-plugins-core clap Catch2::Catch2WithMain)
      target_compile_definitions(clap-plugins-tests PUBLIC -DCATCH_CONFIG_PREFIX_ALL)
      add_test(NAME test-clap-plugins COMMAND clap-plugins-tests)
      add_dependencies(clap-tests clap-plugins-tests)
    else()
        message(STATUS "Catch2 >= 3 isn't found -> disable clap-plugins unit tests")
    endif()
endif()