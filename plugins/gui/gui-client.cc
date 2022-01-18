#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QThread>
#include <QWindow>

#include "../io/messages.hh"
#include "gui-client.hh"

#ifdef _WIN32
#   include <windows.h>
#endif

namespace clap {

   GuiClient::GuiClient(int socket, const QStringList &qmlImportPath, const QUrl &qmlSkin)
      : _quickView(new QQuickView()) {

      _pluginProxy = new PluginProxy(*this);
      _transportProxy = new TransportProxy(*this);

      ////////////////////////
      // I/O initialization //
      ////////////////////////

#if defined(Q_OS_UNIX)
      _remoteChannel.reset(new clap::RemoteChannel(
         [this](const clap::RemoteChannel::Message &msg) { onMessage(msg); },
         false,
         *this,
         socket));

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
      _remoteChannel.reset(new clap::RemoteChannel(
         [this](const clap::RemoteChannel::Message &msg) { onMessage(msg); },
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

   bool GuiClient::spawn() {
      // Nothing to do
   }

   void GuiClient::defineParameter(const clap_param_info &paramInfo) {
      _pluginProxy->defineParameter(paramInfo);
   }

   void GuiClient::updateParameter(clap_id paramId, double value, double modAmount) {
      auto p = _pluginProxy->param(paramId);
      assert(p);
      if (!p)
         return;

      p->setValueFromPlugin(rq.value);
      p->setModulationFromPlugin(rq.modulation);
   }

   void GuiClient::clearTransport() { _transportProxy->clear(); }

   void GuiClient::updateTransport(const clap_event_transport &transport) {
      _transportProxy->update(transport);
   }

   void GuiClient::showLater() {
      return QMetaObject::invokeMethod(
         this,
         [this] {
            if (_hostWindow && _quickView)
               _quickView->show();
         },
         Qt::QueuedConnection);
   }

   bool GuiClient::attachCocoa(void *nsView) {
#ifdef Q_OS_MACOS
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(rq.nsView)));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         _quickView->show();
         return true;
      }
#endif
      return false;
   }

   bool GuiClient::attachWin32(clap_hwnd window) {
#ifdef Q_OS_WIN
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(rq.hwnd)));
      if (_hostWindow) {
         return false;
         _quickView->setParent(_hostWindow.get());
         return showLater();
      }
#endif
      return false;
   }

   bool GuiClient::attachX11(const char *display_name, unsigned long window) {
#ifdef Q_OS_LINUX
      _hostWindow.reset(QWindow::fromWinId(rq.window));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         _quickView->show();
         return true;
      }
#endif
      return false;
   }

   bool GuiClient::size(uint32_t *width, uint32_t *height) {
      if (!_quickView)
         return false;

      auto rootItem = _quickView->rootObject();
      if (!rootItem)
         return false;

      *width = rootItem->width();
      *height = rootItem->height();
   }

   bool GuiClient::setScale(double scale) {}

   bool GuiClient::show() { _quickView->show(); }

   bool GuiClient::hide() { _quickView->hide(); }

   void GuiClient::destroy() {}
} // namespace clap