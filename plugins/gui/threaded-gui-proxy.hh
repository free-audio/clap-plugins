#pragma once

#include <memory>

#include "abstract-gui.hh"

namespace clap {

   class Gui;

   class ThreadedGuiProxy : public AbstractGui {
   public:
      ThreadedGuiProxy(AbstractGuiListener &listener, std::shared_ptr<Gui> &guiClient);
      ~ThreadedGuiProxy() override;

      void addImportPath(const std::string &importPath) override;
      void setSkin(const std::string &skinUrl) override;

      void defineParameter(const clap_param_info &paramInfo) override;
      void updateParameter(clap_id paramId, double value, double modAmount) override;

      void clearTransport() override;
      void updateTransport(const clap_event_transport &transport) override;

      bool openWindow() override;
      bool attachCocoa(void *nsView) override;
      bool attachWin32(clap_hwnd window) override;
      bool attachX11(const char *displayName, unsigned long window) override;

      bool canResize() override;
      bool getSize(uint32_t *width, uint32_t *height) override;
      bool setSize(uint32_t width, uint32_t height) override;
      bool roundSize(uint32_t *width, uint32_t *height) override;
      bool setScale(double scale) override;

      bool show() override;
      bool hide() override;

      void destroy() override;

      auto gui() { return _gui; }

   private:
      std::shared_ptr<Gui> _gui;
   };
} // namespace clap
