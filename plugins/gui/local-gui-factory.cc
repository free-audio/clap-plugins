#include <cassert>
#include <future>

#include <QGuiApplication>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QBasicTimer>

#include "abstract-gui-listener.hh"
#include "gui.hh"
#include "local-gui-factory.hh"

namespace clap {

   std::weak_ptr<LocalGuiFactory> LocalGuiFactory::_instance;

   LocalGuiFactory::LocalGuiFactory() {
      assert(!_app);
      assert(!QCoreApplication::instance());

      _timer = Timer::createNative(1000 / 60, [this] {
         onTimer();
      });

      if (!_timer)
         return;

      _timer->start();

      static int argc = 1;
      char arg0[] = "clap-plugin-gui";
      static char *argv[] = {arg0, nullptr};

      QGuiApplication::setAttribute(Qt::AA_PluginApplication);
      _app = std::make_unique<QGuiApplication>(argc, argv);
      _app->processEvents();
   }

   LocalGuiFactory::~LocalGuiFactory() {
      assert(_clients.empty());

      if (_timer) {
         _timer->stop();
         _timer.reset();
      }

      if (_app) {
         assert(_app->thread() == QThread::currentThread());
         _app->quit();
         _app.reset();
      }
   }

   bool LocalGuiFactory::isValid() const {
      return _timer && _app;
   }

   std::shared_ptr<LocalGuiFactory> LocalGuiFactory::getInstance() {
      auto ptr = _instance.lock();
      if (ptr)
         return ptr;
      ptr = std::make_shared<LocalGuiFactory>();
      if (!ptr->isValid())
         return nullptr;
      _instance = ptr;
      return ptr;
   }

   std::unique_ptr<GuiHandle>
   LocalGuiFactory::createGui(AbstractGuiListener &listener,
                              const std::vector<std::string> &qmlImportPath) {
      assert(_app);

      std::shared_ptr<Gui> ptr;
      assert(_app->thread() == QThread::currentThread());

      QStringList qtQmlImportPath;
      for (auto &s : qmlImportPath)
         qtQmlImportPath.append(QString::fromStdString(s));
      ptr = std::make_shared<Gui>(listener, qtQmlImportPath);

      if (!ptr)
         return nullptr;

      _clients.emplace(&listener, ptr);
      return std::make_unique<GuiHandle>(_instance.lock(), ptr);
   }

   void
   LocalGuiFactory::releaseGui(GuiHandle& handle)
   {
      auto g = dynamic_cast<Gui *>(&handle.gui());
      assert(g);
      if (!g)
         return;

      auto l = &g->guiListener();
      assert(_clients.count(l) == 1);
      _clients.erase(l);
   }

   void LocalGuiFactory::onTimer() {
      assert(_app->thread() == QThread::currentThread());

      _app->processEvents();
      for (auto &it : _clients)
         it.first->onGuiPoll();
   }
} // namespace clap
