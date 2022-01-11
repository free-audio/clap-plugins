#pragma once

#if (defined(__unix__) || defined(__APPLE__))
#   include "posix-remote-channel.hh"

namespace clap {
   using RemoteChannel = PosixRemoteChannel;
}
#else
#   include "win32-remote-channel.hh"

namespace clap {
   using RemoteChannel = Win32RemoteChannel;
}
#endif
