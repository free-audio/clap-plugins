# target_wayland_protocols(<target> <xml>...)
#
# Generates the client code of the given protocols, and adds it to the target.
# The xml paths are relative to the wayland-protocols data directory, for example:
# stable/xdg-shell/xdg-shell.xml
function(target_wayland_protocols target)
  find_package(PkgConfig REQUIRED)
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
  if(NOT WAYLAND_PROTOCOLS_DIR)
    message(FATAL_ERROR "wayland-protocols not found")
  endif()
  find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

  set(output_dir ${CMAKE_CURRENT_BINARY_DIR}/wayland-protocols)
  file(MAKE_DIRECTORY ${output_dir})

  foreach(xml ${ARGN})
    get_filename_component(name ${xml} NAME_WE)
    set(xml ${WAYLAND_PROTOCOLS_DIR}/${xml})
    set(header ${output_dir}/${name}-client-protocol.h)
    set(code ${output_dir}/${name}-protocol.c)
    add_custom_command(
      OUTPUT ${header}
      COMMAND ${WAYLAND_SCANNER} client-header ${xml} ${header}
      DEPENDS ${xml}
      VERBATIM)
    add_custom_command(
      OUTPUT ${code}
      COMMAND ${WAYLAND_SCANNER} private-code ${xml} ${code}
      DEPENDS ${xml}
      VERBATIM)
    target_sources(${target} PRIVATE ${header} ${code})
  endforeach()

  target_include_directories(${target} PRIVATE ${output_dir})
endfunction()
