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
#ifdef _WIN32
   struct RemoteGuiWin32Data final {
      STARTUPINFO _si;
      PROCESS_INFORMATION _childInfo;
   };

   std::string escapeArg(const std::string &s) {
      return "\"" + std::regex_replace(s, std::regex("\""), "\\\"") + "\"";
   }
#endif

   RemoteGui::RemoteGui(CorePlugin &plugin) : AbstractGui(plugin) {}

   RemoteGui::~RemoteGui() {
      if (_channel)
         destroy();

      assert(!_channel);
   }

   void RemoteGui::registerTimer() {
      _timerId = CLAP_INVALID_ID;
      _plugin._host.timerSupportRegister(1000 / 60, &_timerId);
   }



   void RemoteGui::modifyFd(int flags) {
#ifdef __unix
      _plugin._host.posixFdSupportModify(posixFd(), flags);
#endif
   }

   void RemoteGui::removeFd() {
#ifdef __unix
      _plugin._host.posixFdSupportUnregister(posixFd());
#endif
      _plugin._host.timerSupportUnregister(_timerId);
   }

   int RemoteGui::posixFd() const {
#ifdef __unix
      return _channel ? _channel->fd() : -1;
#endif
      return -1;
   }

   void RemoteGui::onPosixFd(int flags) {
      if (flags & CLAP_POSIX_FD_READ)
         _channel->tryReceive();
      if (flags & CLAP_POSIX_FD_WRITE)
         _channel->trySend();
      if (flags & CLAP_POSIX_FD_ERROR)
         _channel->onError();
   }

   void RemoteGui::onMessage(const BasicRemoteChannel::Message &msg) {
      switch (msg.type) {
      case messages::kParamAdjustRequest: {
         messages::ParamAdjustRequest rq;
         msg.get(rq);
         _plugin.guiAdjust(rq.paramId, rq.value, rq.flags);
         break;

      case messages::kSubscribeToTransportRequest: {
         messages::SubscribeToTransportRequest rq;
         msg.get(rq);
         _isTransportSubscribed = rq.isSubscribed;
         break;
      }
      }
      }
   }



} // namespace clap