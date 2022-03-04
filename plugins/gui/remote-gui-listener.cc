#include "../io/remote-channel.hh"

#include "remote-gui-factory.hh"
#include "remote-gui-listener.hh"

namespace clap {

   RemoteGuiListener::RemoteGuiListener(RemoteGuiFactory &clientFactory, uint32_t clientId)
      : _clientFactory(clientFactory), _clientId(clientId) {}

   RemoteGuiListener::~RemoteGuiListener() = default;

   void RemoteGuiListener::onGuiPoll() {}

   void RemoteGuiListener::onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) {
      messages::ParamAdjustRequest rq;
      rq.flags = flags;
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