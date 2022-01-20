#if (defined(__unix__) || defined(__APPLE__))
#   include <fcntl.h>
#   include <sys/socket.h>
#elif defined(_WIN32)
#   include <Windows.h>
#endif

#include <cassert>
#include <iostream>
#include <regex>
#include <sstream>

#include "../io/messages.hh"
#include "core-plugin.hh"
#include "path-provider.hh"
#include "remote-gui.hh"

namespace clap {


   RemoteGui::RemoteGui(CorePlugin &plugin) : AbstractGui(plugin) {}

   RemoteGui::~RemoteGui() {
      if (_channel)
         destroy();

      assert(!_channel);
   }

   void RemoteGui::onPosixFd(int flags) {
      if (flags & CLAP_POSIX_FD_READ)
         _channel->tryReceive();
      if (flags & CLAP_POSIX_FD_WRITE)
         _channel->trySend();
      if (flags & CLAP_POSIX_FD_ERROR)
         _channel->onError();
   }



} // namespace clap