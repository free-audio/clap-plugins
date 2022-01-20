#include "../io/messages.hh"
#include "../io/remote-channel.hh"

#include "abstract-gui-listener.hh"
#include "remote-gui-client-factory-proxy.hh"
#include "remote-gui-client-proxy.hh"

namespace clap {
   void RemoteGuiClientProxy::defineParameter(const clap_param_info &info) {
      messages::DefineParameterRequest rq{info};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiClientProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      messages::ParameterValueRequest rq{paramId, value, modAmount};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   bool RemoteGuiClientProxy::size(uint32_t *width, uint32_t *height) {
      messages::SizeRequest request;
      messages::SizeResponse response;

      if (!_clientFactory._channel->sendRequestSync(_clientId, request, response))
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   bool RemoteGuiClientProxy::setScale(double scale) {
      messages::SetScaleRequest request{scale};
      messages::SetScaleResponse response;

      if (!_clientFactory._channel->sendRequestSync(_clientId, request, response))
         return false;

      return response.succeed;
   }

   bool RemoteGuiClientProxy::show() {
      messages::ShowRequest request;

      return _clientFactory._channel->sendRequestAsync(_clientId, request);
   }

   bool RemoteGuiClientProxy::hide() {
      messages::HideRequest request;

      return _clientFactory._channel->sendRequestAsync(_clientId, request);
   }

   void RemoteGuiClientProxy::destroy() {
      if (!_clientFactory._channel)
         return;

      messages::DestroyRequest request;
      _clientFactory._channel->sendRequestAsync(_clientId, request);
   }

   bool RemoteGuiClientProxy::attachCocoa(void *nsView) {
      messages::AttachCocoaRequest request{nsView};
      messages::AttachResponse response;

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   bool RemoteGuiClientProxy::attachWin32(clap_hwnd window) {
      messages::AttachWin32Request request{window};
      messages::AttachResponse response;

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   bool RemoteGuiClientProxy::attachX11(const char *display_name, unsigned long window) {
      messages::AttachX11Request request{window};
      messages::AttachResponse response;

      std::snprintf(
         request.display, sizeof(request.display), "%s", display_name ? display_name : "");

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   void RemoteGuiClientProxy::clearTransport() {
      messages::UpdateTransportRequest rq{false};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiClientProxy::updateTransport(const clap_event_transport &transport) {
      messages::UpdateTransportRequest rq{true, transport};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiClientProxy::onMessage(const RemoteChannel::Message &msg) {
      assert(msg.clientId == _clientId);

      switch (msg.type) {
      case messages::kParamAdjustRequest: {
         messages::ParamAdjustRequest rq;
         msg.get(rq);
         _listener.onGuiParamAdjust(rq.paramId, rq.value, rq.flags);
         break;

      case messages::kSubscribeToTransportRequest: {
         messages::SubscribeToTransportRequest rq;
         msg.get(rq);
         _listener.onGuiSetTransportIsSubscribed(rq.isSubscribed);
         break;
      }
      }
      }
   }
} // namespace clap