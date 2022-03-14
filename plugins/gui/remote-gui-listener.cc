#include "../io/remote-channel.hh"

#include "remote-gui-factory.hh"
#include "remote-gui-listener.hh"

namespace clap {

   RemoteGuiListener::RemoteGuiListener(RemoteGuiFactory &clientFactory, uint32_t clientId)
      : _clientFactory(clientFactory), _clientId(clientId) {}

   RemoteGuiListener::~RemoteGuiListener() = default;

   void RemoteGuiListener::onGuiPoll() {}

   void RemoteGuiListener::onGuiParamBeginAdjust(clap_id paramId) {
      messages::ParamBeginAdjustRequest rq;
      rq.paramId = paramId;
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiListener::onGuiParamEndAdjust(clap_id paramId) {
      messages::ParamEndAdjustRequest rq;
      rq.paramId = paramId;
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiListener::onGuiParamAdjust(clap_id paramId, double value) {
      messages::ParamAdjustRequest rq;
      rq.paramId = paramId;
      rq.value = value;
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiListener::onGuiSetTransportIsSubscribed(bool isSubscribed) {
      messages::SubscribeToTransportRequest rq;
      rq.isSubscribed = isSubscribed;
      _clientFactory._channel->sendRequestAsync(_clientId, rq);
   }

   void RemoteGuiListener::onGuiWindowClosed(bool wasDestroyed) {
      messages::WindowClosedNotification notification{wasDestroyed};
      _clientFactory._channel->sendRequestAsync(_clientId, notification);
   }
} // namespace clap