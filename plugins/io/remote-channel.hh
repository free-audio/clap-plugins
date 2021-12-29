#pragma once

#ifdef __unix
#   include "posix-remote-channel.hh"

namespace clap {
   using RemoteChannel = PosixRemoteChannel;
}
#endif
