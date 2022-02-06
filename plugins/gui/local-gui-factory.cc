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
#include "gui-client.hh"
#include "local-gui-factory.hh"

namespace clap {

   std::weak_ptr<LocalGuiFactory> LocalGuiFactory::_instance;

   LocalGuiFactory::LocalGuiFactory() {
      assert(!_app);
      assert(!QCoreApplication::instance());

      static int argc = 1;
      char arg0[] = "clap-plugin-gui";
      static char *argv[] = {arg0, nullptr};

      QGuiApplication::setAttribute(Qt::AA_PluginApplication);
      _app = std::make_unique<QGuiApplication>(argc, argv);
      _timer = Timer::createNative(1000 / 60, [this] {
         onTimer();
      });

      _app->processEvents();
   }

   LocalGuiFactory::~LocalGuiFactory() {
      if (_app) {
         // TODO
      }
   }

   std::shared_ptr<LocalGuiFactory> LocalGuiFactory::getInstance() {
      auto ptr = _instance.lock();
      if (ptr)
         return ptr;
      ptr.reset(new LocalGuiFactory());
      _instance = ptr;
      return ptr;
   }

   std::shared_ptr<AbstractGui>
   LocalGuiFactory::createGuiClient(AbstractGuiListener &listener,
                                          const std::vector<std::string> &qmlImportPath,
                                          const std::string &qmlUrl) {
      assert(_app);

      std::shared_ptr<GuiClient> ptr;
      assert(_app->thread() == QThread::currentThread());

      QStringList qtQmlImportPath;
      for (auto &s : qmlImportPath)
         qtQmlImportPath.append(QString::fromStdString(s));
      QUrl qtQmlUrl(QString::fromStdString(qmlUrl));
      ptr = std::make_shared<GuiClient>(listener, qtQmlImportPath, qtQmlUrl);

      if (!ptr)
         return nullptr;

      _clients.emplace(&listener, ptr);
      return ptr;
   }

   void LocalGuiFactory::onTimer() {
      assert(_app->thread() == QThread::currentThread());

      _app->processEvents();
      for (auto &it : _clients)
         it.first->onGuiPoll();
   }
} // namespace clap