cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0100 NEW)  # handle .hh files
project(CLAP_EXAMPLES C CXX)

set(UsePkgConfig TRUE CACHE BOOL "Use PkgConfig to find RtMidi and RtAudio dependencies")

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

include(CPack)

add_custom_target(run-host
    COMMAND $<TARGET_FILE:clap-host> -p $<TARGET_FILE:clap-plugins> -i 0
    DEPENDS clap-host clap-plugins clap-gui
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIRECTORY}")