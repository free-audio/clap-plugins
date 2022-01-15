#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QWindow>
#include <QThread>

#include "../io/messages.hh"
#include "gui-client.hh"

#ifdef _WIN32
#   include <windows.h>
#endif

GuiClient::GuiClient(int socket, const QStringList& qmlImportPath, const QUrl& qmlSkin)
: _quickView(new QQuickView()) {

   _pluginProxy = new PluginProxy(this);
   _transportProxy = new TransportProxy(this);

   ////////////////////////
   // I/O initialization //
   ////////////////////////

#if defined(Q_OS_UNIX)
   _remoteChannel.reset(new clap::RemoteChannel(
      [this](const clap::RemoteChannel::Message &msg) { onMessage(msg); }, false, *this, socket));

   _socketReadNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Read, this));
   connect(_socketReadNotifier.get(),
           &QSocketNotifier::activated,
           [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
              _remoteChannel->tryReceive();
              if (!_remoteChannel->isOpen())
                 QCoreApplication::quit();
           });

   _socketWriteNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Write, this));
   connect(_socketWriteNotifier.get(),
           &QSocketNotifier::activated,
           [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
              _remoteChannel->trySend();
              if (!_remoteChannel->isOpen()) {
                 QCoreApplication::quit();
              }
           });

   _socketReadNotifier->setEnabled(true);
   _socketWriteNotifier->setEnabled(false);
#elif defined(Q_OS_WINDOWS)
   _remoteChannel.reset(
      new clap::RemoteChannel([this](const clap::RemoteChannel::Message &msg) { onMessage(msg); },
                              false,
                              pipeInHandle,
                              pipeOutHandle,
                              [] { QCoreApplication::quit(); }));
#endif

   ////////////////////////
   // QML initialization //
   ////////////////////////

   auto qmlContext = _quickView->engine()->rootContext();
   for (const auto &str : qmlImportPath)
      _quickView->engine()->addImportPath(str);
   qmlContext->setContextProperty("plugin", _pluginProxy);
   qmlContext->setContextProperty("transport", _transportProxy);

   _quickView->setSource(qmlSkin);
}

void GuiClient::modifyFd(int flags) {
   _socketReadNotifier->setEnabled(flags & CLAP_POSIX_FD_READ);
   _socketWriteNotifier->setEnabled(flags & CLAP_POSIX_FD_WRITE);
}

void GuiClient::removeFd() {
   _socketReadNotifier.reset();
   _socketWriteNotifier.reset();
   QCoreApplication::quit();
}

void GuiClient::onMessage(const clap::RemoteChannel::Message &msg) {
   switch (msg.type) {
   case clap::messages::kDestroyRequest:
      clap::messages::DestroyResponse rp;
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      QCoreApplication::quit();
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

   case clap::messages::kSetScaleRequest: {
      clap::messages::SetScaleRequest rq;
      clap::messages::SetScaleResponse rp{false};

      msg.get(rq);
      // We ignore it.
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachX11Request: {
      clap::messages::AttachX11Request rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);

#ifdef Q_OS_LINUX
      _hostWindow.reset(QWindow::fromWinId(rq.window));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         _quickView->show();
         sync();
         rp.succeed = true;
      }
#endif

      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachWin32Request: {
      clap::messages::AttachWin32Request rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);

#ifdef Q_OS_WIN
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(rq.hwnd)));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         rp.succeed = true;
      }
#endif

      _remoteChannel->sendResponseAsync(rp, msg.cookie);

      if (_hostWindow && _quickView)
         _quickView->show();
      break;
   }

   case clap::messages::kAttachCocoaRequest: {
      clap::messages::AttachCocoaRequest rq;
      clap::messages::AttachResponse rp{false};

      msg.get(rq);

#ifdef Q_OS_MACOS
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(rq.nsView)));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         _quickView->show();
         rp.succeed = true;
      }
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
