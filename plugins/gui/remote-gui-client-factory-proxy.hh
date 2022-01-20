#include <thread>

#include "../io/remote-channel.hh"

#include "abstract-gui-client-factory.hh"

namespace clap {

   class RemoteGuiClientProxy;

   class RemoteGuiClientFactoryProxy : public AbstractGuiClientFactory {
   public:
      RemoteGuiClientFactoryProxy();
      ~RemoteGuiClientFactoryProxy() override;

      static std::shared_ptr<RemoteGuiClientFactoryProxy> getInstance();

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) override;

   private:
      friend class RemoteGuiClientProxy;

      static std::weak_ptr<RemoteGuiClientFactoryProxy> _instance;

      void run();

      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<RemoteChannel> _channel;
      std::unordered_map<AbstractGuiListener *, std::weak_ptr<RemoteGuiClientProxy>> _clients;
   };
} // namespace clap