#ifdef __unix__
#   include <fcntl.h>
#   include <sys/socket.h>
#elif defined(_WIN32)
#   include <Windows.h>
#endif

#include <cassert>
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

   bool RemoteGui::spawn() {
#ifdef __unix
      assert(_child == -1);
#elif defined(_WIN32)
      assert(!_data);
#endif
      assert(!_channel);

      static const constexpr size_t KPIPE_BUFSZ = 128 * 1024;

      if (!_plugin._host.canUseTimerSupport() || !_plugin._host.canUseFdSupport())
         return false;

      auto &pathProvider = _plugin.pathProvider();
      auto path = pathProvider.getGuiExecutable();
      auto skin = pathProvider.getSkinDirectory();
      auto qmlLib = pathProvider.getQmlLibDirectory();

      printf("About to start GUI: %s --skin %s --qml-import %s\n",
             path.c_str(),
             skin.c_str(),
             qmlLib.c_str());

#ifdef __unix__
      /* create a socket pair */
      int sockets[2];
      if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets)) {
         return false;
      }

      _child = ::fork();
      if (_child == -1) {
         ::close(sockets[0]);
         ::close(sockets[1]);
         return false;
      }

      if (_child == 0) {
         // Child
         ::close(sockets[0]);
         char socketStr[16];
         ::snprintf(socketStr, sizeof(socketStr), "%d", sockets[1]);
         ::execl(path.c_str(),
                 path.c_str(),
                 "--socket",
                 socketStr,
                 "--skin",
                 skin.c_str(),
                 "--qml-import",
                 qmlLib.c_str(),
                 (const char *)nullptr);
         printf("Failed to start child process: %m\n");
         std::terminate();
      } else {
         // Parent
         ::close(sockets[1]);
      }

      _timerId = CLAP_INVALID_ID;
      _plugin._host.timerSupportRegisterTimer(1000 / 60, &_timerId);
      _plugin._host.fdSupportRegisterFD(sockets[0], CLAP_FD_READ | CLAP_FD_ERROR);
      _channel.reset(new RemoteChannel(
         [this](const RemoteChannel::Message &msg) { onMessage(msg); }, true, *this, sockets[0]));

      return true;
#else
      std::ostringstream cmdline;
      HANDLE pluginToGuiPipes[2];
      HANDLE guiToPluginPipes[2];
      SECURITY_ATTRIBUTES secAttrs;
      char cmdlineBuffer[32 * 1024];

      secAttrs.nLength = sizeof(secAttrs);
      secAttrs.lpSecurityDescriptor = nullptr;
      secAttrs.bInheritHandle = true;

      _data = std::make_unique<RemoteGuiWin32Data>();
      memset(&_data->_si, 0, sizeof(_data->_si));
      memset(&_data->_childInfo, 0, sizeof(_data->_childInfo));

      if (!CreatePipe(&pluginToGuiPipes[0], &pluginToGuiPipes[1], &secAttrs, KPIPE_BUFSZ))
         goto fail0;

      if (!CreatePipe(&guiToPluginPipes[0], &guiToPluginPipes[1], &secAttrs, KPIPE_BUFSZ))
         goto fail1;

      if (!SetHandleInformation(&pluginToGuiPipes[1], HANDLE_FLAG_INHERIT, 0))
         goto fail2;

      if (!SetHandleInformation(&guiToPluginPipes[0], HANDLE_FLAG_INHERIT, 0))
         goto fail2;

      cmdline << escapeArg(path) << " --skin " << escapeArg(skin) << " --qml-import "
              << escapeArg(qmlLib) << " --pipe-in "
              << reinterpret_cast<uintptr_t>(pluginToGuiPipes[0]) << " --pipe-out "
              << reinterpret_cast<uintptr_t>(guiToPluginPipes[1]);
      snprintf(cmdlineBuffer, sizeof(cmdlineBuffer), "%s", cmdline.str().c_str());

      if (!CreateProcess("clap-gui",
                         cmdlineBuffer,
                         nullptr,
                         nullptr,
                         true,
                         NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT,
                         nullptr,
                         nullptr,
                         &_data->_si,
                         &_data->_childInfo))
         goto fail2;

      CloseHandle(pluginToGuiPipes[1]);
      CloseHandle(guiToPluginPipes[0]);

      _channel = std::make_unique<RemoteChannel>(
         [this](const RemoteChannel::Message &msg) { onMessage(msg); },
         true,
         pluginToGuiPipes[0],
         guiToPluginPipes[1]);
      return true;

   fail2:
      CloseHandle(guiToPluginPipes[0]);
      CloseHandle(guiToPluginPipes[1]);
   fail1:
      CloseHandle(pluginToGuiPipes[0]);
      CloseHandle(pluginToGuiPipes[1]);
   fail0:
      _data.reset();
      return false;
#endif
   }

   void RemoteGui::modifyFd(clap_fd_flags flags) {
#ifdef __unix
      _plugin._host.fdSupportModifyFD(fd(), flags);
#endif
   }

   void RemoteGui::removeFd() {
#ifdef __unix
      _plugin._host.fdSupportUnregisterFD(fd());
#endif
      _plugin._host.timerSupportUnregisterTimer(_timerId);
   }

   int RemoteGui::fd() const {
#ifdef __unix
      return _channel ? _channel->fd() : -1;
#endif
      return -1;
   }

   void RemoteGui::onFd(clap_fd_flags flags) {
      if (flags & CLAP_FD_READ)
         _channel->tryReceive();
      if (flags & CLAP_FD_WRITE)
         _channel->trySend();
      if (flags & CLAP_FD_ERROR)
         _channel->onError();
   }

   void RemoteGui::onMessage(const BasicRemoteChannel::Message &msg) {
      switch (msg.type) {
      case messages::kAdjustRequest: {
         messages::AdjustRequest rq;
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

   void RemoteGui::defineParameter(const clap_param_info &info) noexcept {
      _channel->sendRequestAsync(messages::DefineParameterRequest{info});
   }

   bool RemoteGui::size(uint32_t *width, uint32_t *height) noexcept {
      messages::SizeRequest request;
      messages::SizeResponse response;

      if (!_channel->sendRequestSync(request, response))
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   void RemoteGui::setScale(double scale) noexcept {
      _channel->sendRequestAsync(messages::SetScaleRequest{scale});
   }

   bool RemoteGui::show() noexcept {
      messages::ShowRequest request;
      messages::ShowResponse response;

      return _channel->sendRequestSync(request, response);
   }

   bool RemoteGui::hide() noexcept {
      messages::HideRequest request;
      messages::HideResponse response;

      return _channel->sendRequestSync(request, response);
   }

   void RemoteGui::destroy() noexcept {
      if (!_channel)
         return;

      messages::DestroyRequest request;
      messages::DestroyResponse response;

      _channel->sendRequestSync(request, response);
      _channel->close();
      _channel.reset();

      waitChild();
   }

   void RemoteGui::waitChild() {
#ifdef __unix__
      if (_child == -1)
         return;
      int stat = 0;
      int ret;

      do {
         ret = ::waitpid(_child, &stat, 0);
      } while (ret == -1 && errno == EINTR);

      _child = -1;
#endif
   }

   bool RemoteGui::attachCocoa(void *nsView) noexcept {
      messages::AttachCocoaRequest request{nsView};
      messages::AttachResponse response;

      return _channel->sendRequestSync(request, response);
   }

   bool RemoteGui::attachWin32(clap_hwnd window) noexcept {
      messages::AttachWin32Request request{window};
      messages::AttachResponse response;

      return _channel->sendRequestSync(request, response);
   }

   bool RemoteGui::attachX11(const char *display_name, unsigned long window) noexcept {
      messages::AttachX11Request request;
      messages::AttachResponse response;

      request.window = window;
      std::snprintf(
         request.display, sizeof(request.display), "%s", display_name ? display_name : "");

      return _channel->sendRequestSync(request, response);
   }

   void RemoteGui::onTimer() {
      _plugin._pluginToGuiQueue.consume(
         [this](clap_id paramId, const CorePlugin::PluginToGuiValue &value) {
            messages::ParameterValueRequest rq{paramId, value.value, value.mod};
            _channel->sendRequestAsync(rq);
         });

      if (_isTransportSubscribed && _plugin._hasTransportCopy) {
         messages::UpdateTransportRequest rq{_plugin._hasTransport, _plugin._transportCopy};
         _channel->sendRequestAsync(rq);
         _plugin._hasTransportCopy = false;
      }
   }

} // namespace clap