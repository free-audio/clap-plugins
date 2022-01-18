#pragma once

#ifdef __unix__
#   include <sys/wait.h>
#endif

#include <memory>

#include "abstract-gui.hh"
#include "remote-channel.hh"

namespace clap {
   struct RemoteGuiWin32Data;
   class RemoteGui : public AbstractGui, public RemoteChannel::EventControl
   {
   public:
      RemoteGui(AbstractGuiListener &listener);
      ~RemoteGui();

      bool spawn() noexcept override;

      void defineParameter(const clap_param_info &) noexcept override;
      void updateParameter(clap_id paramId, double value, double modAmount) noexcept override;

      void clearTransport() override;
      void updateTransport(const clap_event_transport &transport) override;

      bool attachCocoa(void *nsView) noexcept override;
      bool attachWin32(clap_hwnd window) noexcept override;
      bool attachX11(const char *display_name, unsigned long window) noexcept override;

      bool size(uint32_t *width, uint32_t *height) noexcept override;
      bool setScale(double scale) noexcept override;

      bool show() noexcept override;
      bool hide() noexcept override;

      void destroy() noexcept override;

      // RemoteChannel::EventControl
      void modifyFd(int flags) override;
      void removeFd() override;

      int posixFd() const;
      void onPosixFd(int flags);

   private:
      void onMessage(const RemoteChannel::Message &msg);
      void waitChild();

      std::unique_ptr<RemoteChannel> _channel;

#if (defined(__unix__) || defined(__APPLE__))
      pid_t _child = -1;
#elif defined(_WIN32)
      std::unique_ptr<RemoteGuiWin32Data> _data;
#endif
   };
} // namespace clap