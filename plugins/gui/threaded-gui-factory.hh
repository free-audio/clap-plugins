#pragma once

#include <memory>
#include <thread>
#include <unordered_map>

#include "abstract-gui-client-factory.hh"

class QThread;
class QGuiApplication;
class QTimer;

namespace clap {
   // Has a dedicated thread for running QGuiApplication
   class ThreadedGuiClientFactory : public AbstractGuiClientFactory {
   public:
      ThreadedGuiClientFactory();
      ~ThreadedGuiClientFactory() override;

      static std::shared_ptr<ThreadedGuiClientFactory> getInstance();

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) override;

   private:
      void onTimer();

      static std::weak_ptr<ThreadedGuiClientFactory> _instance;

      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<QGuiApplication> _app;
      std::unordered_map<AbstractGuiListener *, std::weak_ptr<AbstractGui>> _clients;
      std::unique_ptr<QTimer> _timer;
   };
} // namespace clap