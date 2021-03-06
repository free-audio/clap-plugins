#pragma once

#include <thread>
#include <queue>
#include <mutex>

#include "../io/remote-channel.hh"

#include "abstract-gui-factory.hh"

namespace clap {

   class RemoteGuiProxy;

   class RemoteGuiFactoryProxy : public AbstractGuiFactory, public RemoteChannel::EventControl {
   public:
      RemoteGuiFactoryProxy(const std::string &guiPath);
      ~RemoteGuiFactoryProxy() override;

      static std::shared_ptr<RemoteGuiFactoryProxy> getInstance(const std::string &guiPath);

      std::unique_ptr<GuiHandle> createGui(AbstractGuiListener &listener) override;

      void releaseGui(GuiHandle &handle) override;

      void exec(const std::function<void()>& cb);
      void execAsync(std::function<void()> cb);

   private:
      friend class RemoteGuiProxy;

      static std::weak_ptr<RemoteGuiFactoryProxy> _instance;

      void run();
      void runCallbacks();
      void runGuiPoll();

      bool spawnChild();
      void waitChild();

      void onMessage(const RemoteChannel::Message &msg);

      // RemoteChannel::EventControl
      void modifyFd(int flags) override { _pollFlags = flags; }
      void removeFd() override {}

      const std::string _guiPath;
      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<RemoteChannel> _channel;

      bool _quit = false;

      std::unordered_map<AbstractGuiListener *, std::weak_ptr<RemoteGuiProxy>> _clients;
      std::unordered_map<uint32_t /* clientId */, std::weak_ptr<RemoteGuiProxy>> _clientIdMap;

      std::recursive_mutex _callbacksLock;
      std::queue<std::function<void()>> _callbacks;

#if (defined(__unix__) || defined(__APPLE__))
      void posixLoop();

      pid_t _child = -1;
      int _pollFlags = 0;

#elif defined(_WIN32)
      void windowsLoop();

      std::unique_ptr<RemoteGuiWin32Data> _data;
#endif
   };
} // namespace clap