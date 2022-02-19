#include "../io/messages.hh"
#include "../io/remote-channel.hh"

#include "abstract-gui-listener.hh"
#include "remote-gui-factory-proxy.hh"
#include "remote-gui-proxy.hh"

namespace clap {
   RemoteGuiProxy::RemoteGuiProxy(AbstractGuiListener &listener,
                                  RemoteGuiFactoryProxy &factory,
                                  uint32_t clientId)
      : super(listener), _clientFactory(factory), _clientId(clientId) {}

   void RemoteGuiProxy::addImportPath(const std::string &importPath)
   {
      messages::AddImportPathRequest rq;

      snprintf(rq.importPath, sizeof (rq.importPath), "%s", importPath.c_str());
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiProxy::setSkin(const std::string &skinUrl)
   {
      messages::SetSkinRequest rq;

      snprintf(rq.skinUrl, sizeof (rq.skinUrl), "%s", skinUrl.c_str());
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiProxy::defineParameter(const clap_param_info &info) {
      messages::DefineParameterRequest rq{info};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      messages::ParameterValueRequest rq{paramId, value, modAmount};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   bool RemoteGuiProxy::canResize() {
      messages::CanResizeRequest request;
      messages::CanResizeResponse response;

      if (!_clientFactory._channel->sendRequestSync(_clientId, request, response))
         return false;

      return response.succeed;
   }

   bool RemoteGuiProxy::size(uint32_t *width, uint32_t *height) {
      messages::SizeRequest request;
      messages::SizeResponse response;

      if (!_clientFactory._channel->sendRequestSync(_clientId, request, response))
         return false;

      if (!response.succeed)
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   bool RemoteGuiProxy::roundSize(uint32_t *width, uint32_t *height) {
      messages::RoundSizeRequest request;
      messages::RoundSizeResponse response;

      request.width = *width;
      request.height = *height;

      if (!_clientFactory._channel->sendRequestSync(_clientId, request, response))
         return false;

      if (!response.succeed)
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   bool RemoteGuiProxy::setScale(double scale) {
      messages::SetScaleRequest request{scale};
      messages::SetScaleResponse response;

      if (!_clientFactory._channel->sendRequestSync(_clientId, request, response))
         return false;

      return response.succeed;
   }

   bool RemoteGuiProxy::show() {
      messages::ShowRequest request;

      return _clientFactory._channel->sendRequestAsync(_clientId, request);
   }

   bool RemoteGuiProxy::hide() {
      messages::HideRequest request;

      return _clientFactory._channel->sendRequestAsync(_clientId, request);
   }

   void RemoteGuiProxy::destroy() {
      if (!_clientFactory._channel)
         return;

      messages::DestroyRequest request;
      _clientFactory._channel->sendRequestAsync(_clientId, request);
   }

   bool RemoteGuiProxy::openWindow() {
      messages::OpenWindowRequest request;
      messages::AttachResponse response;

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   bool RemoteGuiProxy::attachCocoa(void *nsView) {
      messages::AttachCocoaRequest request{nsView};
      messages::AttachResponse response;

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   bool RemoteGuiProxy::attachWin32(clap_hwnd window) {
      messages::AttachWin32Request request{window};
      messages::AttachResponse response;

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   bool RemoteGuiProxy::attachX11(const char *display_name, unsigned long window) {
      messages::AttachX11Request request{window};
      messages::AttachResponse response;

      std::snprintf(
         request.display, sizeof(request.display), "%s", display_name ? display_name : "");

      return _clientFactory._channel->sendRequestSync(_clientId, request, response);
   }

   void RemoteGuiProxy::clearTransport() {
      messages::UpdateTransportRequest rq{false};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiProxy::updateTransport(const clap_event_transport &transport) {
      messages::UpdateTransportRequest rq{true, transport};
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiProxy::onMessage(const RemoteChannel::Message &msg) {
      assert(msg.header.clientId == _clientId);

      switch (msg.header.type) {
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