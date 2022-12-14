#pragma once

#include <QVariant>
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

   class Gui final : public QObject, public AbstractGui {
      Q_OBJECT;

   public:
      Gui(AbstractGuiListener &listener);
      ~Gui() override;

      Gui(const Gui &) = delete;
      Gui(Gui &&) = delete;
      Gui &operator=(const Gui &) = delete;
      Gui &operator=(Gui &&) = delete;

      void addImportPath(const std::string &importPath) override;
      void setSkin(const std::string &skinPath) override;

      void defineParameter(const clap_param_info &paramInfo) override;
      void updateParameter(clap_id paramId, double value, double modAmount) override;
      void setParameterMappingIndication(clap_id paramId,
                                         bool hasIndication,
                                         clap_color color,
                                         const char *label,
                                         const char *description) override;
      void setParameterAutomationIndication(clap_id paramId,
                                            uint32_t automationState,
                                            clap_color color) override;

      void clearTransport() override;
      void updateTransport(const clap_event_transport &transport) override;

      bool openWindow() override;

      bool attachCocoa(clap_nsview nsView) override;
      bool attachWin32(clap_hwnd window) override;
      bool attachX11(clap_xwnd window) override;

      bool setTransientX11(clap_xwnd window) override;
      bool setTransientWin32(clap_hwnd window) override;
      bool setTransientCocoa(clap_nsview nsView) override;

      bool canResize() override;
      bool getSize(uint32_t *width, uint32_t *height) override;
      bool setSize(uint32_t width, uint32_t hight) override;
      bool roundSize(uint32_t *width, uint32_t *height) override;
      bool setScale(double scale) override;

      bool show() override;
      bool hide() override;

      void destroy() override;

      auto &guiListener() const { return _listener; }

   private:
      void showLater();
      void setRootScale(double scale);

      static bool wantsLogicalSize() noexcept;
      void onQuickViewVisibilityChange(bool isVisible);

      // Qt windows
      std::unique_ptr<QQuickView> _quickView;
      std::unique_ptr<QWindow> _hostWindow;
      bool _isFloating = false;

      // QML proxy objects
      std::unique_ptr<PluginProxy> _pluginProxy;
      std::unique_ptr<TransportProxy> _transportProxy;

      double _rootScale = 1.;
   };

} // namespace clap