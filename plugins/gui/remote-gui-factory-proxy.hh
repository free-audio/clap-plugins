#pragma once

#include <thread>

#include "../io/remote-channel.hh"

#include "abstract-gui-factory.hh"

namespace clap {

   class RemoteGuiProxy;

   class RemoteGuiFactoryProxy : public AbstractGuiFactory, public RemoteChannel::EventControl {
   public:
      RemoteGuiFactoryProxy(const std::string& guiPath);
      ~RemoteGuiFactoryProxy() override;

      static std::shared_ptr<RemoteGuiFactoryProxy> getInstance(const std::string& guiPath);

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) override;

   private:
      friend class RemoteGuiProxy;

      static std::weak_ptr<RemoteGuiFactoryProxy> _instance;

      void run();

      bool spawnChild();
      void waitChild();

      void onMessage(const RemoteChannel::Message &msg);

      // RemoteChannel::EventControl
      void modifyFd(int flags) override { _pollFlags = flags; }
      void removeFd() override {}

      const std::string _guiPath;
      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<RemoteChannel> _channel;

      std::unordered_map<AbstractGuiListener *, std::weak_ptr<RemoteGuiProxy>> _clients;
      std::unordered_map<uint32_t /* clientId */, std::weak_ptr<RemoteGuiProxy>> _clientIdMap;

#if (defined(__unix__) || defined(__APPLE__))
      pid_t _child = -1;
      int _pollFlags = 0;
#elif defined(_WIN32)
      std::unique_ptr<RemoteGuiWin32Data> _data;
#endif
   };
} // namespace clap