#pragma once

#include <memory>
#include <thread>
#include <unordered_map>

#include <QObject>

#include "abstract-gui-factory.hh"
#include "native-timer.hh"

QT_BEGIN_NAMESPACE
class QThread;
class QGuiApplication;
class QBasicTimer;
QT_END_NAMESPACE

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
      std::unique_ptr<NativeTimer> _timer;
   };

} // namespace clap