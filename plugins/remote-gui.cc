#ifdef __unix__
#   include <fcntl.h>
#   include <sys/socket.h>
#elif defined(_WIN32)
#   include <Windows.h>
#endif

#include <cassert>
#include <regex>
#include <sstream>
#include <iostream>

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

      if (!_plugin._host.canUseTimerSupport() || !_plugin._host.canUsePosixFdSupport())
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
      _plugin._host.timerSupportRegister(1000 / 60, &_timerId);
      _plugin._host.posixFdSupportRegister(sockets[0], CLAP_POSIX_FD_READ | CLAP_POSIX_FD_ERROR);
      _channel.reset(new RemoteChannel(
         [this](const RemoteChannel::Message &msg) { onMessage(msg); }, true, *this, sockets[0]));

      return true;
#else
      std::ostringstream cmdline;
      HANDLE pluginToGuiPipe;
      HANDLE guiToPluginPipe;
      SECURITY_ATTRIBUTES secAttrs;
      char buffer[32 * 1024];
      char pipeInPath[256];
      char pipeOutPath[256];
      static int counter{0};

      snprintf(pipeInPath,
               sizeof(pipeInPath),
               "\\\\.\\pipe\\clap-plugtogui.%08x.%08x",
               GetCurrentProcessId(),
               ++counter);

      snprintf(pipeOutPath,
               sizeof(pipeOutPath),
               "\\\\.\\pipe\\clap-guitoplug.%08x.%08x",
               GetCurrentProcessId(),
               counter);

      secAttrs.nLength = sizeof(secAttrs);
      secAttrs.lpSecurityDescriptor = nullptr;
      secAttrs.bInheritHandle = true;

      _data = std::make_unique<RemoteGuiWin32Data>();
      memset(&_data->_si, 0, sizeof(_data->_si));
      memset(&_data->_childInfo, 0, sizeof(_data->_childInfo));

      pluginToGuiPipe = CreateNamedPipe(pipeInPath,
                                        PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                                        PIPE_TYPE_BYTE | PIPE_WAIT,
                                        1,
                                        KPIPE_BUFSZ,
                                        KPIPE_BUFSZ,
                                        0,
                                        nullptr);
      if (!pluginToGuiPipe)
         goto fail0;

      guiToPluginPipe = CreateNamedPipe(pipeOutPath,
                                        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                        PIPE_TYPE_BYTE | PIPE_WAIT,
                                        1,
                                        KPIPE_BUFSZ,
                                        KPIPE_BUFSZ,
                                        0,
                                        nullptr);
      if (!guiToPluginPipe)
         goto fail1;

      cmdline << escapeArg(path) << " --skin " << escapeArg(skin) << " --qml-import "
              << escapeArg(qmlLib) << " --pipe-in " << pipeInPath << " --pipe-out " << pipeOutPath;

      snprintf(buffer, sizeof(buffer), "%s", cmdline.str().c_str());

      if (!CreateProcess(nullptr,
                         buffer,
                         nullptr,
                         nullptr,
                         true,
                         NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT | STARTF_USESTDHANDLES,
                         nullptr,
                         nullptr,
                         &_data->_si,
                         &_data->_childInfo))
         goto fail2;

      ConnectNamedPipe(guiToPluginPipe, nullptr);
      ConnectNamedPipe(pluginToGuiPipe, nullptr);

      _channel = std::make_unique<RemoteChannel>(
         [this](const RemoteChannel::Message &msg) { onMessage(msg); },
         true,
         guiToPluginPipe,
         pluginToGuiPipe);
      return true;

   fail2:
      CloseHandle(guiToPluginPipe);
   fail1:
      CloseHandle(pluginToGuiPipe);
   fail0:
      _data.reset();
      return false;
#endif
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