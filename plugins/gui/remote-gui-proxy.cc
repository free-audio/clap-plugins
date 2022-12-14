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

   void RemoteGuiProxy::addImportPath(const std::string &importPath) {
      messages::AddImportPathRequest rq;
      snprintf(rq.importPath, sizeof(rq.importPath), "%s", importPath.c_str());

      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::setSkin(const std::string &skinUrl) {
      messages::SetSkinRequest rq;
      snprintf(rq.skinUrl, sizeof(rq.skinUrl), "%s", skinUrl.c_str());

      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::defineParameter(const clap_param_info &info) {
      messages::DefineParameterRequest rq{info};
      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      messages::ParameterValueRequest rq{paramId, value, modAmount};
      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::setParameterMappingIndication(clap_id paramId,
                                                      bool hasIndication,
                                                      clap_color color,
                                                      const char *label,
                                                      const char *description) {
      messages::SetParameterMappingIndicationRequest rq{paramId, hasIndication, color};
      snprintf(rq.label, sizeof(rq.label), "%s", label);
      snprintf(rq.description, sizeof(rq.description), "%s", description);
      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::setParameterAutomationIndication(clap_id paramId,
                                                         uint32_t automationState,
                                                         clap_color color) {
      messages::SetParameterAutomationIndicationRequest rq{paramId, automationState, color};
      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   bool RemoteGuiProxy::canResize() {
      bool sent = false;
      messages::CanResizeRequest request;
      messages::CanResizeResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      if (!sent)
         return false;

      return response.succeed;
   }

   bool RemoteGuiProxy::getSize(uint32_t *width, uint32_t *height) {
      bool sent = false;
      messages::GetSizeRequest request;
      messages::GetSizeResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      if (!sent)
         return false;

      if (!response.succeed)
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   bool RemoteGuiProxy::setSize(uint32_t width, uint32_t height) {
      bool sent = false;
      messages::SetSizeRequest request{width, height};
      messages::SetSizeResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      if (!sent)
         return false;

      return response.succeed;
   }

   bool RemoteGuiProxy::roundSize(uint32_t *width, uint32_t *height) {
      bool sent = false;
      messages::RoundSizeRequest request;
      messages::RoundSizeResponse response;

      request.width = *width;
      request.height = *height;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      if (!sent)
         return false;

      if (!response.succeed)
         return false;

      *width = response.width;
      *height = response.height;
      return true;
   }

   bool RemoteGuiProxy::setScale(double scale) {
      bool sent = false;
      messages::SetScaleRequest request{scale};
      messages::SetScaleResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      if (!sent)
         return false;

      return response.succeed;
   }

   bool RemoteGuiProxy::show() {
      bool sent = false;
      messages::ShowRequest request;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestAsync(_clientId, request); });

      return sent;
   }

   bool RemoteGuiProxy::hide() {
      bool sent = false;
      messages::HideRequest request;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestAsync(_clientId, request); });

      return sent;
   }

   void RemoteGuiProxy::destroy() {
      if (!_clientFactory._channel)
         return;

      _clientFactory.exec([&] {
         messages::DestroyClientRequest request;
         _clientFactory._channel->sendRequestAsync(_clientId, request);
      });
   }

   bool RemoteGuiProxy::openWindow() {
      bool sent = false;
      messages::OpenWindowRequest request;
      messages::AttachResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   bool RemoteGuiProxy::attachCocoa(clap_nsview nsView) {
      bool sent = false;
      messages::AttachCocoaRequest request{nsView};
      messages::AttachResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   bool RemoteGuiProxy::attachWin32(clap_hwnd window) {
      bool sent = false;
      messages::AttachWin32Request request{window};
      messages::AttachResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   bool RemoteGuiProxy::attachX11(clap_xwnd window) {
      bool sent = false;
      messages::AttachX11Request request{window};
      messages::AttachResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   bool RemoteGuiProxy::setTransientCocoa(clap_nsview nsView) {
      bool sent = false;
      messages::SetTransientCocoaRequest request{nsView};
      messages::SetTransientResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   bool RemoteGuiProxy::setTransientWin32(clap_hwnd window) {
      bool sent = false;
      messages::SetTransientWin32Request request{window};
      messages::SetTransientResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   bool RemoteGuiProxy::setTransientX11(clap_xwnd window) {
      bool sent = false;
      messages::SetTransientX11Request request{window};
      messages::SetTransientResponse response;

      _clientFactory.exec(
         [&] { sent = _clientFactory._channel->sendRequestSync(_clientId, request, response); });

      return sent;
   }

   void RemoteGuiProxy::clearTransport() {
      messages::UpdateTransportRequest rq{false};

      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::updateTransport(const clap_event_transport &transport) {
      messages::UpdateTransportRequest rq{true, transport};
      _clientFactory.exec([&] { _clientFactory._channel->sendRequestAsync(_clientId, rq); });
   }

   void RemoteGuiProxy::onMessage(const RemoteChannel::Message &msg) {
      assert(msg.header.clientId == _clientId);

      switch (msg.header.type) {
      case messages::kParamBeginAdjustRequest: {
         messages::ParamBeginAdjustRequest rq;
         msg.get(rq);
         _listener.onGuiParamBeginAdjust(rq.paramId);
         break;
      }

      case messages::kParamEndAdjustRequest: {
         messages::ParamEndAdjustRequest rq;
         msg.get(rq);
         _listener.onGuiParamEndAdjust(rq.paramId);
         break;
      }

      case messages::kParamAdjustRequest: {
         messages::ParamAdjustRequest rq;
         msg.get(rq);
         _listener.onGuiParamAdjust(rq.paramId, rq.value);
         break;
      }

      case messages::kSubscribeToTransportRequest: {
         messages::SubscribeToTransportRequest rq;
         msg.get(rq);
         _listener.onGuiSetTransportIsSubscribed(rq.isSubscribed);
         break;
      }

      case messages::kWindowClosedNotification: {
         messages::WindowClosedNotification notification;
         msg.get(notification);
         _listener.onGuiWindowClosed(notification.wasDestroyed);
         break;
      }
      }
   }
} // namespace clap
