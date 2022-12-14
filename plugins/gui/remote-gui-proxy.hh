#pragma once

#include "../io/remote-channel.hh"

#include "abstract-gui.hh"

namespace clap {
   class RemoteGuiFactoryProxy;
   class RemoteGuiProxy : public AbstractGui {
      using super = AbstractGui;

   public:
      RemoteGuiProxy(AbstractGuiListener &listener,
                     RemoteGuiFactoryProxy &factory,
                     uint32_t clientId);

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
      bool setSize(uint32_t width, uint32_t height) override;
      bool roundSize(uint32_t *width, uint32_t *height) override;
      bool setScale(double scale) override;

      bool show() override;
      bool hide() override;

      void destroy() override;

   private:
      friend class RemoteGuiFactoryProxy;

      void onMessage(const RemoteChannel::Message &msg);

      RemoteGuiFactoryProxy &_clientFactory;
      const uint32_t _clientId;
   };
} // namespace clap