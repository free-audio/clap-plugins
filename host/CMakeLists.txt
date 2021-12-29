set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(UsePkgConfig)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RtMidi REQUIRED IMPORTED_TARGET rtmidi)
  pkg_check_modules(RtAudio REQUIRED IMPORTED_TARGET rtaudio)
else()
  find_package(RtMidi CONFIG REQUIRED)
  find_package(RtAudio CONFIG REQUIRED)
endif()

find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)

add_executable(clap-host
  application.cc
  application.hh
  audio-settings.cc
  audio-settings.hh
  audio-settings-widget.cc
  audio-settings-widget.hh

  plugin-param.cc
  plugin-param.hh
  plugin-host.cc
  plugin-host.hh
  plugin-quick-control-widget.cc
  plugin-quick-control-widget.hh
  plugin-quick-controls-widget.cc
  plugin-quick-controls-widget.hh

  CMakeLists.txt
  device-reference.hh
  engine.cc
  engine.hh
  main.cc
  main-window.cc
  main-window.hh
  midi-settings.cc
  midi-settings.hh
  midi-settings-widget.cc
  midi-settings-widget.hh
  plugin-info.hh
  plugin-parameters-widget.cc
  plugin-parameters-widget.hh
  settings.cc
  settings-dialog.cc
  settings-dialog.hh
  settings.hh
  settings-widget.cc
  settings-widget.hh
  )

set_target_properties(clap-host PROPERTIES CXX_STANDARD 17)
target_link_libraries(clap-host Qt6::Widgets Qt6::Core)
if(UsePkgConfig)
  target_link_libraries(clap-host PkgConfig::RtMidi PkgConfig::RtAudio)
else()
  target_link_libraries(clap-host RtAudio::rtaudio RtMidi::rtmidi)
endif()
target_link_libraries(clap-host clap-core clap-helpers)

if (LINUX)
  target_link_libraries(clap-host dl pthread)
endif()

if (APPLE)
  # Unclear why this was here, but it breaks ARM builds.
  # set_target_properties(clap-host PROPERTIES OSX_ARCHITECTURES x86_64)

  find_library(CORE_FOUNDATION CoreFoundation)
  find_library(CORE_AUDIO CoreAudio)
  find_library(CORE_AUDIO CoreServices)
  find_library(CORE_MIDI CoreMIDI)
  find_library(AUDIO_UNIT AudioUnit)
  find_library(AUDIO_TOOLBOX AudioToolbox)
  find_library(CARBON Carbon)
  target_link_libraries(clap-host ${CARBON} ${AUDIO_UNIT} ${AUDIO_TOOLBOX} ${CORE_MIDI} ${CORE_AUDIO} ${CORE_SERVICES} ${CORE_FOUNDATION})
endif()

install(TARGETS clap-host DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
