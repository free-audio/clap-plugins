#include <stdexcept>

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

   GuiClient::GuiClient(AbstractGuiListener &listener,
                        const QStringList &qmlImportPath,
                        const QUrl &qmlSkin)
      : AbstractGui(listener), _quickView(new QQuickView()) {

      _pluginProxy = new PluginProxy(*this);
      _transportProxy = new TransportProxy(*this);

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

   void GuiClient::defineParameter(const clap_param_info &paramInfo) {
      _pluginProxy->defineParameter(paramInfo);
   }

   void GuiClient::updateParameter(clap_id paramId, double value, double modAmount) {
      auto p = _pluginProxy->param(paramId);
      assert(p);
      if (!p)
         return;

      p->setValueFromPlugin(value);
      p->setModulationFromPlugin(modAmount);
   }

   void GuiClient::clearTransport() { _transportProxy->clear(); }

   void GuiClient::updateTransport(const clap_event_transport &transport) {
      _transportProxy->update(transport);
   }

   void GuiClient::showLater() {
      QMetaObject::invokeMethod(
         this,
         [this] {
            if (_hostWindow && _quickView)
               _quickView->show();
         },
         Qt::QueuedConnection);
   }

   bool GuiClient::attachCocoa(void *nsView) {
#ifdef Q_OS_MACOS
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(nsView)));
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
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(window)));
      if (_hostWindow) {
         return false;
         _quickView->setParent(_hostWindow.get());
         return showLater();
      }
#endif
      return false;
   }

   bool GuiClient::attachX11(const char *displayName, unsigned long window) {
#ifdef Q_OS_LINUX
      // TODO: check the displayName
      _hostWindow.reset(QWindow::fromWinId(window));
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
      return true;
   }

   bool GuiClient::setScale(double scale) { return false; }

   bool GuiClient::show() {
      _quickView->show();
      return true;
   }

   bool GuiClient::hide() {
      _quickView->hide();
      return true;
   }

   void GuiClient::destroy() {}
} // namespace clap