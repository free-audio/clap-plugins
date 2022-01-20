#include "../io/remote-channel.hh"
#include "../io/messages.hh"

#include "remote-gui-client-proxy.hh"

namespace clap {
   void RemoteGuiClientProxy::defineParameter(const clap_param_info &info) {
      _channel->sendRequestAsync(messages::DefineParameterRequest{info});
   }

   void RemoteGuiClientProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      messages::ParameterValueRequest rq{paramId, value, modAmount};
      _channel->sendRequestAsync(rq);
   }

   bool RemoteGuiClientProxy::size(uint32_t *width, uint32_t *height) {
      messages::SizeRequest request;
      messages::SizeResponse response;

      if (!_channel->sendRequestSync(request, response))
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   bool RemoteGuiClientProxy::setScale(double scale) {
      messages::SetScaleRequest request{scale};
      messages::SetScaleResponse response;

      if (!_channel->sendRequestSync(request, response))
         return false;

      return response.succeed;
   }

   bool RemoteGuiClientProxy::show() {
      messages::ShowRequest request;

      return _channel->sendRequestAsync(request);
   }

   bool RemoteGuiClientProxy::hide() {
      messages::HideRequest request;

      return _channel->sendRequestAsync(request);
   }

   void RemoteGuiClientProxy::destroy() {
      if (!_channel)
         return;

      messages::DestroyRequest request;
      _channel->sendRequestAsync(request);
      _channel->close();
      _channel.reset();

      waitChild();
   }

   void RemoteGuiClientProxy::waitChild() {
#ifdef __unix__
      if (_child == -1)
         return;
      int stat = 0;
      int ret;

      do {
         ret = ::waitpid(_child, &stat, 0);
      } while (ret == -1 && errno == EINTR);

      _child = -1;

#elif defined(_WIN32)

      if (!_data)
         return;

      WaitForSingleObject(_data->_childInfo.hProcess, INFINITE);
      _data.reset();
#endif
   }

   bool RemoteGuiClientProxy::attachCocoa(void *nsView) noexcept {
      messages::AttachCocoaRequest request{nsView};
      messages::AttachResponse response;

      return _channel->sendRequestSync(request, response);
   }

   bool RemoteGuiClientProxy::attachWin32(clap_hwnd window) noexcept {
      messages::AttachWin32Request request{window};
      messages::AttachResponse response;

      return _channel->sendRequestSync(request, response);
   }

   bool RemoteGuiClientProxy::attachX11(const char *display_name, unsigned long window) noexcept {
      messages::AttachX11Request request;
      messages::AttachResponse response;

      request.window = window;
      std::snprintf(
         request.display, sizeof(request.display), "%s", display_name ? display_name : "");

      return _channel->sendRequestSync(request, response);
   }

   void RemoteGuiClientProxy::clearTransport() {
      messages::UpdateTransportRequest rq{false};
      _channel->sendRequestAsync(rq);
   }

   void RemoteGuiClientProxy::updateTransport(const clap_event_transport &transport) {
      messages::UpdateTransportRequest rq{true, transport};
      _channel->sendRequestAsync(rq);
   }
} // namespace clap