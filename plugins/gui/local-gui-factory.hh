#pragma once

#include <memory>
#include <thread>
#include <unordered_map>

#include "abstract-gui-factory.hh"

class QThread;
class QGuiApplication;
class QTimer;

namespace clap {
   class GuiClient;

   // Creates the QGuiApplication on the host's main thread
   class LocalGuiFactory : public AbstractGuiFactory {
   public:
      LocalGuiFactory();
      ~LocalGuiFactory() override;

      static std::shared_ptr<LocalGuiFactory> getInstance();

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) override;

   private:
      void onTimer();

      static std::weak_ptr<LocalGuiFactory> _instance;

      std::unique_ptr<QGuiApplication> _app;
      std::unordered_map<AbstractGuiListener *, std::weak_ptr<GuiClient>> _clients;
      std::unique_ptr<QTimer> _timer;
   };
} // namespace clap