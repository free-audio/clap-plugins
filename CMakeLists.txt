cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0091 NEW) # use CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0100 NEW) # handle .hh files

enable_testing()

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

project(CLAP_EXAMPLES C CXX)

set(UsePkgConfig TRUE CACHE BOOL "Use PkgConfig to find RtMidi and RtAudio dependencies")

find_package(Boost REQUIRED COMPONENTS serialization iostreams)

find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(Qt6Qml CONFIG REQUIRED)
find_package(Qt6Quick CONFIG REQUIRED)
find_package(Qt6QuickControls2 CONFIG REQUIRED)

if(UsePkgConfig)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RtMidi REQUIRED IMPORTED_TARGET rtmidi)
  pkg_check_modules(RtAudio REQUIRED IMPORTED_TARGET rtaudio)
else()
  find_package(RtMidi CONFIG REQUIRED)
  find_package(RtAudio CONFIG REQUIRED)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang|Clang|GNU")
  add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
endif()

add_subdirectory(clap EXCLUDE_FROM_ALL)
add_subdirectory(clap-helpers EXCLUDE_FROM_ALL)
add_subdirectory(host)
add_subdirectory(plugins)

###########
## CPACK ##
###########

set(CPACK_PACKAGE_NAME "clap-examples")
set(CPACK_PACKAGE_VENDOR "CLAP")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CLAP Reference Plugins and Host")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/free-audio/clap-examples")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "clap-examples")
set(CPACK_NSIS_MANIFEST_DPI_AWARE TRUE)
set(CPACK_NSIS_plugins_INSTALL_DIRECTORY "$COMMONPROGRAMDATA/CLAP")

include(CPack)

add_custom_target(run-host
    COMMAND $<TARGET_FILE:clap-host> -p $<TARGET_FILE:clap-plugins> -i 0
    DEPENDS clap-host clap-plugins
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIRECTORY}")