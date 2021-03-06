#pragma once

#include <memory>
#include <thread>
#include <unordered_map>

#include <QObject>

#include "abstract-gui-factory.hh"
#include "timer.hh"

QT_BEGIN_NAMESPACE
class QThread;
class QGuiApplication;
class QBasicTimer;
QT_END_NAMESPACE

namespace clap {
   class Gui;

   // Creates the QGuiApplication on the host's main thread
   class LocalGuiFactory : public AbstractGuiFactory {
   public:
      LocalGuiFactory();
      ~LocalGuiFactory() override;

      static std::shared_ptr<LocalGuiFactory> getInstance();

      std::unique_ptr<GuiHandle> createGui(AbstractGuiListener &listener) override;

      void releaseGui(GuiHandle &gui) override;

   private:
      bool isValid() const;
      void onTimer();

      static std::shared_ptr<LocalGuiFactory> _instance;

      std::unique_ptr<QGuiApplication> _app;
      std::unordered_map<AbstractGuiListener *, std::weak_ptr<Gui>> _clients;
      std::unique_ptr<Timer> _timer;
   };

} // namespace clap