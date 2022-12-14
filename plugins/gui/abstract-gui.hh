#pragma once

#include <clap/clap.h>

#include <string>

namespace clap {

   class CorePlugin;
   class AbstractGuiListener;
   class AbstractGui {
   public:
      AbstractGui(AbstractGuiListener &listener);
      virtual ~AbstractGui();

      virtual void addImportPath(const std::string& importPath) = 0;
      virtual void setSkin(const std::string& skinUrl) = 0;

      virtual void defineParameter(const clap_param_info &paramInfo) = 0;
      virtual void updateParameter(clap_id paramId, double value, double modAmount) = 0;
      virtual void setParameterIndication(clap_id paramId, bool hasIndication, clap_color color) = 0;

      virtual void clearTransport() = 0;
      virtual void updateTransport(const clap_event_transport &transport) = 0;

      [[nodiscard]] virtual bool openWindow() = 0;

      [[nodiscard]] virtual bool attachCocoa(clap_nsview nsView) = 0;
      [[nodiscard]] virtual bool attachWin32(clap_hwnd window) = 0;
      [[nodiscard]] virtual bool attachX11(clap_xwnd window) = 0;

      virtual bool setTransientX11(clap_xwnd window) = 0;
      virtual bool setTransientWin32(clap_hwnd window) = 0;
      virtual bool setTransientCocoa(clap_nsview nsView) = 0;

      [[nodiscard]] virtual bool canResize() = 0;
      [[nodiscard]] virtual bool getSize(uint32_t *width, uint32_t *height) = 0;
      [[nodiscard]] virtual bool setSize(uint32_t width, uint32_t height) = 0;
      [[nodiscard]] virtual bool roundSize(uint32_t *width, uint32_t *height) = 0;
      [[nodiscard]] virtual bool setScale(double scale) = 0;

      virtual bool show() = 0;
      virtual bool hide() = 0;

      virtual void destroy() = 0;

      AbstractGuiListener& listener() const { return _listener; }

   protected:
      AbstractGuiListener &_listener;

      bool _isTransportSubscribed = false;
   };

} // namespace clap