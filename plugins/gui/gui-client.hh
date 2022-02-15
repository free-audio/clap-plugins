#pragma once

#include <QWindow>

#include "abstract-gui.hh"
#include "plugin-proxy.hh"
#include "transport-proxy.hh"

QT_BEGIN_NAMESPACE
class QQuickView;
class QUrl;
class QWindow;
QT_END_NAMESPACE

namespace clap {

   class AbstractGui;
   class AbstractGuiListener;

   class GuiClient : public QObject, public AbstractGui {
      Q_OBJECT;

   public:
      GuiClient(AbstractGuiListener &listener, const QStringList &qmlImportPath);
      ~GuiClient() override;

      GuiClient(const GuiClient &) = delete;
      GuiClient(GuiClient &&) = delete;
      GuiClient &operator=(const GuiClient &) = delete;
      GuiClient &operator=(GuiClient &&) = delete;

      void setSkin(const std::string &skinPath) override;

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