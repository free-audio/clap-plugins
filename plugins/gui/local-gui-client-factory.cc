#include <cassert>

#include <QGuiApplication>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "gui-client.hh"
#include "local-gui-client-factory.hh"

namespace clap {

   LocalGuiClientFactory::LocalGuiClientFactory() {
      _thread.reset(new std::thread(&LocalGuiClientFactory::run, this));
   }

   LocalGuiClientFactory::~LocalGuiClientFactory() {
      if (_app) {
         assert(_thread);
      }

      if (_thread) {
         assert(std::this_thread::get_id() != _thread->get_id());
         _thread->join();
         _thread.reset();
      }
   }

   std::shared_ptr<LocalGuiClientFactory> LocalGuiClientFactory::getInstance() {
      auto ptr = _instance.lock();
      if (ptr)
         return ptr;
      ptr.reset(new LocalGuiClientFactory());
      _instance = ptr;
      return ptr;
   }

   void LocalGuiClientFactory::run() {
      static int argc = 1;
      char arg0[] = "clap-plugin-gui";
      static char *argv[] = {arg0, nullptr};

      assert(!_app);
      _app = std::make_unique<QGuiApplication>(argc, argv);
      _app->exec();
   }

   std::shared_ptr<AbstractGui>
   LocalGuiClientFactory::createGuiClient(AbstractGuiListener &listener,
                                          const std::vector<std::string> &qmlImportPath,
                                          const std::string &qmlUrl) {
      assert(_app);
      assert(_thread);

      std::shared_ptr<AbstractGui> ptr;
      QMetaObject::invokeMethod(
         this,
         [&] {
            QStringList qtQmlImportPath;
            for (auto &s : qmlImportPath)
               qtQmlImportPath.append(QString::fromStdString(s));
            QUrl qtQmlUrl(QString::fromStdString(qmlUrl));
            ptr = std::make_shared<GuiClient>(listener, qtQmlImportPath, qtQmlUrl);

            if (ptr)
               _clients.emplace(&listener, ptr);
         },
         Qt::BlockingQueuedConnection);

      return ptr;
   }
} // namespace clap