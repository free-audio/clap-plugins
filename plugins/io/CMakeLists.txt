if(UNIX)
    set(CLAP_IO_SRC
        posix-remote-channel.hh
        posix-remote-channel.cc)
elseif(WIN32)
    set(CLAP_IO_SRC
        win32-remote-channel.hh
        win32-remote-channel.cc)
endif()

add_library(clap-io STATIC
    basic-remote-channel.hh
    basic-remote-channel.cc
    remote-channel.hh
    buffer.hh

    ${CLAP_IO_SRC}
)

target_link_libraries(clap-io clap-core)
set_target_properties(clap-io PROPERTIES CXX_STANDARD 20)
set_target_properties(clap-io PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(clap-io INTERFACE .)