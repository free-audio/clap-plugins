#if (defined(__unix__) || defined(__APPLE__))
#   include <fcntl.h>
#   include <poll.h>
#   include <sys/socket.h>
#   include <sys/wait.h>
#   include <unistd.h>
#elif defined(_WIN32)
#   include <Windows.h>
#endif

#include <cassert>
#include <future>
#include <iostream>
#include <regex>
#include <sstream>

#include "../io/messages.hh"
#include "../io/remote-channel.hh"

#include "abstract-gui-listener.hh"
#include "remote-gui-factory-proxy.hh"
#include "remote-gui-proxy.hh"

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

   std::weak_ptr<RemoteGuiFactoryProxy> RemoteGuiFactoryProxy::_instance;

   RemoteGuiFactoryProxy::RemoteGuiFactoryProxy(const std::string &guiPath) : _guiPath(guiPath) {

      if (!spawnChild()) {
         _quit = true;
         return;
      }

      assert(!_thread);
      _thread = std::make_unique<std::thread>([this] { run(); });
   }

   RemoteGuiFactoryProxy::~RemoteGuiFactoryProxy() {
      if (_thread && _thread->joinable()) {
         exec([this] {
            messages::DestroyRequest rq;
            _channel->sendRequestAsync(0, rq);
         });
         _quit = true;
         _thread->join();
         _thread.reset();
      }
   }

   std::shared_ptr<RemoteGuiFactoryProxy>
   RemoteGuiFactoryProxy::getInstance(const std::string &guiPath) {
      auto ptr = _instance.lock();
      if (ptr)
         return ptr;
      ptr.reset(new RemoteGuiFactoryProxy(guiPath));
      _instance = ptr;
      return ptr;
   }

   std::unique_ptr<GuiHandle> RemoteGuiFactoryProxy::createGui(AbstractGuiListener &listener) {
      messages::CreateClientRequest rq;
      messages::CreateClientResponse rp;

      if (!_channel->sendRequestSync(0, rq, rp))
         return nullptr;

      auto ptr = std::make_shared<RemoteGuiProxy>(listener, *this, rp.clientId);

      _clientIdMap.emplace(rp.clientId, ptr);
      _clients.emplace(&listener, ptr);

      return std::make_unique<GuiHandle>(_instance.lock(), ptr);
   }

   void RemoteGuiFactoryProxy::releaseGui(GuiHandle &handle) {
      auto g = dynamic_cast<RemoteGuiProxy *>(&handle.gui());
      auto l = &g->listener();

      _clients.erase(l);
      _clientIdMap.erase(g->_clientId);
   }

   bool RemoteGuiFactoryProxy::spawnChild() {
#if (defined(__unix__) || defined(__APPLE__))
      assert(_child == -1);
#elif defined(_WIN32)
      assert(!_data);
#endif
      assert(!_channel);

      static const constexpr size_t KPIPE_BUFSZ = 128 * 1024;

#if (defined(__unix__) || defined(__APPLE__))
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
         printf("About to start GUI: %s --socket %d\n", _guiPath.c_str(), sockets[1]);

         ::close(sockets[0]);
         char socketStr[16];
         ::snprintf(socketStr, sizeof(socketStr), "%d", sockets[1]);
         ::execl(_guiPath.c_str(), _guiPath.c_str(), "--socket", socketStr, nullptr);
         printf("Failed to start child process: %m\n");
         std::terminate();
      } else {
         // Parent
         ::close(sockets[1]);
      }

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

      cmdline << escapeArg(path) << " --pipe-in " << pipeInPath << " --pipe-out " << pipeOutPath;

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

   void RemoteGuiFactoryProxy::waitChild() {
#ifdef __unix__
      if (_child == -1)
         return;
      int stat = 0;
      int ret;

      do {
         ret = ::waitpid(_child, &stat, 0);
      } while (ret == -1 && errno == EINTR);

      _child = -1;

#elif defined(_WIN32)

      if (!_data)
         return;

      WaitForSingleObject(_data->_childInfo.hProcess, INFINITE);
      _data.reset();
#endif
   }

   void RemoteGuiFactoryProxy::onMessage(const RemoteChannel::Message &msg) {
      auto it = _clientIdMap.find(msg.header.clientId);
      if (it == _clientIdMap.end())
         return;

      auto proxy = it->second.lock();
      if (!proxy)
         return;

      proxy->onMessage(msg);
   }

   void RemoteGuiFactoryProxy::execAsync(std::function<void()> cb) {
      if (_thread->get_id() == std::this_thread::get_id())
      {
         cb();
         return;
      }

      std::lock_guard<std::recursive_mutex> guard(_callbacksLock);
      _callbacks.push(std::move(cb));
   }

   void RemoteGuiFactoryProxy::exec(const std::function<void()> &cb) {
      if (_thread->get_id() == std::this_thread::get_id())
      {
         cb();
         return;
      }

      std::promise<void> promise;
      execAsync([&cb, &promise] {
         cb();
         promise.set_value();
      });
      promise.get_future().wait();
   }

   void RemoteGuiFactoryProxy::run() {
#if (defined(__unix__) || defined(__APPLE__))
      posixLoop();
#elif defined(_WIN32)
      windowsLoop();
#else
#   error "unsupported target"
#endif

      _channel.reset();
      waitChild();
   }

   void RemoteGuiFactoryProxy::runCallbacks() {
      std::lock_guard<std::recursive_mutex> guard(_callbacksLock);
      while (!_callbacks.empty()) {
         _callbacks.front()();
         _callbacks.pop();
      }
   }

   void RemoteGuiFactoryProxy::runGuiPoll() {
      for (auto &client : _clients)
         client.first->onGuiPoll();
   }

#if (defined(__unix__) || defined(__APPLE__))
   void RemoteGuiFactoryProxy::posixLoop() {
      while (!_quit && _channel->isOpen()) {
         runCallbacks();
         runGuiPoll();

         pollfd pfd;
         pfd.fd = _channel->fd();
         pfd.events = POLLIN;
         pfd.revents = 0;

         if (_pollFlags & CLAP_POSIX_FD_WRITE)
            pfd.events |= POLLOUT;
         if (_pollFlags & CLAP_POSIX_FD_ERROR)
            pfd.events |= POLLERR;

         auto ret = poll(&pfd, 1, 10);
         if (ret < 0) {
            if (errno == EAGAIN || errno == EINTR || errno == ETIMEDOUT)
               continue;
            auto errStr = strerror(errno);
            std::cerr << "[clap-plugins] poll(): " << errStr << std::endl;
            return;
         }

         if (ret == 0)
            // hmmm why?
            continue;

         if (pfd.revents & POLLIN)
            _channel->tryReceive();
         if (pfd.revents & POLLOUT && _channel)
            _channel->trySend();
         if (pfd.revents & POLLERR && _channel)
            _channel->onError();
      }
   }
#endif
} // namespace clap
