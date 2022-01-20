#pragma once

#include <thread>

#include "../io/remote-channel.hh"

#include "abstract-gui-client-factory.hh"

namespace clap {

   class RemoteGuiClientProxy;

   class RemoteGuiClientFactoryProxy : public AbstractGuiClientFactory, public RemoteChannel::EventControl {
   public:
      RemoteGuiClientFactoryProxy(const std::string& guiPath);
      ~RemoteGuiClientFactoryProxy() override;

      static std::shared_ptr<RemoteGuiClientFactoryProxy> getInstance(const std::string& guiPath);

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) override;

   private:
      friend class RemoteGuiClientProxy;

      static std::weak_ptr<RemoteGuiClientFactoryProxy> _instance;

      void run();

      bool spawnChild();
      void waitChild();

      void onMessage(const RemoteChannel::Message &msg);

      // RemoteChannel::EventControl
      void modifyFd(int flags) override;
      void removeFd() override;

      const std::string _guiPath;
      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<RemoteChannel> _channel;

      std::unordered_map<AbstractGuiListener *, std::weak_ptr<RemoteGuiClientProxy>> _clients;
      std::unordered_map<uint32_t /* clientId */, std::weak_ptr<RemoteGuiClientProxy>> _clientIdMap;

#if (defined(__unix__) || defined(__APPLE__))
      pid_t _child = -1;
#elif defined(_WIN32)
      std::unique_ptr<RemoteGuiWin32Data> _data;
#endif
   };
} // namespace clap