#include <memory>
#include <thread>

#include <QObject>

#include "abstract-gui-client-factory.hh"

class QThread;
class QGuiApplication;

namespace clap {
   class LocalGuiClientFactory : public QObject, public AbstractGuiClientFactory {
      Q_OBJECT

   public:
      LocalGuiClientFactory();
      ~LocalGuiClientFactory() override;

      static std::shared_ptr<LocalGuiClientFactory> getInstance();

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) override;

   private:
      static std::weak_ptr<LocalGuiClientFactory> _instance;

      void run();

      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<QGuiApplication> _app;
      std::unordered_map<AbstractGuiListener *, std::weak_ptr<AbstractGui>> _clients;
   };
} // namespace clap