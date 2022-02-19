#pragma once

#include <memory>
#include <thread>
#include <unordered_map>

#include <QtGlobal>

#include "abstract-gui-factory.hh"

QT_BEGIN_NAMESPACE
class QThread;
class QGuiApplication;
class QTimer;
QT_END_NAMESPACE

namespace clap {
   // Has a dedicated thread for running QGuiApplication
   class ThreadedGuiFactory : public AbstractGuiFactory {
   public:
      ThreadedGuiFactory();
      ~ThreadedGuiFactory() override;

      static std::shared_ptr<ThreadedGuiFactory> getInstance();

      std::unique_ptr<GuiHandle> createGui(AbstractGuiListener &listener) override;

      void releaseGui(GuiHandle &handle) override;

   private:
      void onTimer();

      static std::shared_ptr<ThreadedGuiFactory> _instance;

      std::unique_ptr<std::thread> _thread;
      std::unique_ptr<QGuiApplication> _app;
      std::unordered_map<AbstractGuiListener *, std::weak_ptr<AbstractGui>> _clients;
      std::unique_ptr<QTimer> _timer;
   };
} // namespace clap
