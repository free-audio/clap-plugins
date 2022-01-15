#pragma once

#ifdef __unix__
#   include <sys/wait.h>
#endif

#include <memory>

#include "abstract-gui.hh"
#include "remote-channel.hh"

namespace clap {
   class GuiProxy : public AbstractGui, public RemoteChannel::EventControl
   {
   public:
      GuiProxy(CorePlugin &plugin);
      ~GuiProxy();

      virtual std::unique_ptr<RemoteChannel> spawnRemote() = 0;
      virtual void waitRemote() = 0;

      void defineParameter(const clap_param_info &) noexcept final;
      void updateParameter(clap_id paramId, double value, double modAmount) noexcept final;

      bool attachCocoa(void *nsView) noexcept final;
      bool attachWin32(clap_hwnd window) noexcept final;
      bool attachX11(const char *display_name, unsigned long window) noexcept final;

      bool size(uint32_t *width, uint32_t *height) noexcept final;
      bool setScale(double scale) noexcept final;

      bool show() noexcept final;
      bool hide() noexcept final;

      void destroy() noexcept override;

      // RemoteChannel::EventControl
      void modifyFd(int flags) final;
      void removeFd() final;

      int posixFd() const;
      void onPosixFd(int flags);

      void registerTimer();
      clap_id timerId() const noexcept { return _timerId; }
      void onTimer();

   private:
      void onMessage(const RemoteChannel::Message &msg);

      std::unique_ptr<RemoteChannel> _channel;

      clap_id _timerId = CLAP_INVALID_ID;
   };
} // namespace clap