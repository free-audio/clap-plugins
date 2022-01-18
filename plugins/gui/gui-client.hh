#pragma once

#include <QGuiApplication>
#include <QSocketNotifier>
#include <QWindow>

#include "abstract-gui.hh"
#include "plugin-proxy.hh"
#include "transport-proxy.hh"

class QQuickView;
class QUrl;

namespace clap {

   class AbstractGui;
   class AbstractGuiListener;

   class GuiClient : public QObject, public AbstractGui {
      Q_OBJECT;
      using super = QGuiApplication;

   public:
      GuiClient(AbstractGuiListener &listener,
                const QStringList &qmlImportPath,
                const QUrl &qmlSkin);

      bool spawn() override;

      void defineParameter(const clap_param_info &paramInfo) override;
      void updateParameter(clap_id paramId, double value, double modAmount) override;

      void clearTransport() override;
      void updateTransport(const clap_event_transport &transport) override;

      bool attachCocoa(void *nsView) override;
      bool attachWin32(clap_hwnd window) override;
      bool attachX11(const char *displayName, unsigned long window) override;

      bool size(uint32_t *width, uint32_t *height) override;
      bool setScale(double scale) override;

      bool show() override;
      bool hide() override;

      void destroy() override;

      auto &guiListener() const { return _listener; }

   private:
      void showLater();

      // Qt windows
      QQuickView *_quickView = nullptr;
      std::unique_ptr<QWindow> _hostWindow = nullptr;

      // QML proxy objects
      PluginProxy *_pluginProxy = nullptr;
      TransportProxy *_transportProxy = nullptr;
   };

} // namespace clap