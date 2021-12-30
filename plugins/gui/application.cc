#include <QCommandLineParser>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QWindow>

#include <iostream>

#include "../io/messages.hh"
#include "application.hh"

#include <windows.h>

Application::Application(int &argc, char **argv)
   : QGuiApplication(argc, argv), _quickView(new QQuickView()) {

   bool waitForDebbugger = false;
   while (waitForDebbugger)
      ;

   QCommandLineParser parser;

   QCommandLineOption skinOpt("skin", tr("path to the skin directory"), tr("path"));
   QCommandLineOption qmlLibOpt("qml-import", tr("QML import path"), tr("path"));

#ifdef Q_OS_UNIX
   QCommandLineOption socketOpt("socket", tr("socket fd"), tr("path"));
   parser.addOption(socketOpt);
#endif

#ifdef Q_OS_WINDOWS
   QCommandLineOption pipeInOpt("pipe-in", tr("input pipe handle"), tr("path"));
   QCommandLineOption pipeOutOpt("pipe-out", tr("output pipe handle"), tr("path"));
   parser.addOption(pipeInOpt);
   parser.addOption(pipeOutOpt);
#endif

   parser.addOption(skinOpt);
   parser.addOption(qmlLibOpt);
   parser.addHelpOption();

   parser.process(*this);

   qmlRegisterType<ParameterProxy>("org.clap", 1, 0, "ParameterProxy");
   qmlRegisterType<TransportProxy>("org.clap", 1, 0, "TransportProxy");
   qmlRegisterType<PluginProxy>("org.clap", 1, 0, "PluginProxy");

   _pluginProxy = new PluginProxy(this);
   _transportProxy = new TransportProxy(this);

   ////////////////////////
   // I/O initialization //
   ////////////////////////

#ifdef Q_OS_UNIX
   auto socket = parser.value(socketOpt).toULongLong();

   _remoteChannel.reset(new clap::RemoteChannel(
      [this](const clap::RemoteChannel::Message &msg) { onMessage(msg); }, false, *this, socket));

   _socketReadNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Read, this));
   connect(_socketReadNotifier.get(),
           &QSocketNotifier::activated,
           [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
              _remoteChannel->tryReceive();
              if (!_remoteChannel->isOpen())
                 quit();
           });

   _socketWriteNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Write, this));
   connect(_socketWriteNotifier.get(),
           &QSocketNotifier::activated,
           [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
              _remoteChannel->trySend();
              if (!_remoteChannel->isOpen()) {
                 quit();
              }
           });

   _socketErrorNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Exception, this));
   connect(_socketErrorNotifier.get(),
           &QSocketNotifier::activated,
           [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
              _remoteChannel->onError();
              quit();
           });

   _socketReadNotifier->setEnabled(true);
   _socketWriteNotifier->setEnabled(false);
   _socketErrorNotifier->setEnabled(false);
#endif

#ifdef Q_OS_WINDOWS

   auto pipeInName = parser.value(pipeInOpt).toStdString();
   auto pipeOutName = parser.value(pipeOutOpt).toStdString();

   auto pipeInHandle = CreateFileA(pipeInName.c_str(),
                                   GENERIC_READ,
                                   0,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                   NULL);

   auto pipeOutHandle = CreateFileA(pipeOutName.c_str(),
                                    GENERIC_WRITE,
                                    0,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                    NULL);

   std::cout << "Inside clap-gui " << pipeInName << ", " << pipeOutName << std::endl;

   _remoteChannel.reset(
      new clap::RemoteChannel([this](const clap::RemoteChannel::Message &msg) { onMessage(msg); },
                              false,
                              pipeInHandle,
                              pipeOutHandle));
#endif

   ////////////////////////
   // QML initialization //
   ////////////////////////

   auto qmlContext = _quickView->engine()->rootContext();
   for (const auto &str : parser.values(qmlLibOpt))
      _quickView->engine()->addImportPath(str);
   qmlContext->setContextProperty("plugin", _pluginProxy);
   qmlContext->setContextProperty("transport", _transportProxy);

   _quickView->setSource(QUrl::fromLocalFile(parser.value(skinOpt) + "/main.qml"));
}

void Application::modifyFd(clap_fd_flags flags) {
   _socketReadNotifier->setEnabled(flags & CLAP_FD_READ);
   _socketWriteNotifier->setEnabled(flags & CLAP_FD_WRITE);
   _socketErrorNotifier->setEnabled(flags & CLAP_FD_ERROR);
}

void Application::removeFd() {
   _socketReadNotifier.reset();
   _socketWriteNotifier.reset();
   _socketErrorNotifier.reset();
   quit();
}

void Application::onMessage(const clap::RemoteChannel::Message &msg) {
   std::cout << "[GUI] received msg: " << msg.type << std::endl;
   switch (msg.type) {
   case clap::messages::kDestroyRequest:
      clap::messages::DestroyResponse rp;
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      quit();
      break;

   case clap::messages::kUpdateTransportRequest: {
      clap::messages::UpdateTransportRequest rq;
      msg.get(rq);
      _transportProxy->update(rq.hasTransport, rq.transport);
      break;
   }

   case clap::messages::kDefineParameterRequest: {
      clap::messages::DefineParameterRequest rq;
      msg.get(rq);
      _pluginProxy->defineParameter(rq.info);
      break;
   }

   case clap::messages::kParameterValueRequest: {
      clap::messages::ParameterValueRequest rq;
      msg.get(rq);
      auto p = _pluginProxy->param(rq.paramId);
      p->setValueFromPlugin(rq.value);
      p->setModulationFromPlugin(rq.modulation);
      break;
   }

   case clap::messages::kSizeRequest: {
      clap::messages::SizeResponse rp;
      auto rootItem = _quickView->rootObject();
      rp.width = rootItem ? rootItem->width() : 500;
      rp.height = rootItem ? rootItem->height() : 300;
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachX11Request: {
      clap::messages::AttachX11Request rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);

#ifdef Q_OS_LINUX
      _hostWindow.reset(QWindow::fromWinId(rq.window));
      _quickView->setParent(_hostWindow.get());
      _quickView->show();
      sync();
      rp.succeed = true;
#endif

      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachWin32Request: {
      clap::messages::AttachWin32Request rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);

#ifdef Q_OS_WIN
      std::cout << "QWindow::winId: " << reinterpret_cast<WId>(rq.hwnd) << std::endl;
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(rq.hwnd)));
      if (_hostWindow) {
         std::cout << "setParent" << std::endl;
         _quickView->setParent(_hostWindow.get());
         std::cout << "show" << std::endl;
         sync();
         _quickView->show();
         std::cout << "sync" << std::endl;
         sync();
         std::cout << "finished" << std::endl;
         rp.succeed = true;
      }
#endif

      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachCocoaRequest: {
      clap::messages::AttachCocoaRequest rq;
      clap::messages::AttachResponse rp{false};

      msg.get(rq);

#ifdef Q_OS_MACOS
      hostWindow_.reset(QWindow::fromWinId(rq.nsView));
      quickView_->setParent(hostWindow_.get());
      _quickView->show();
      sync();
      rp.succeed = true;
#endif

      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kShowRequest: {
      _quickView->show();
      clap::messages::ShowResponse rp;
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kHideRequest: {
      _quickView->hide();
      clap::messages::HideResponse rp;
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }
   }
}
