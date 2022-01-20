#include "remote-gui-listener.hh"
#include "../io/remote-channel.hh"

namespace clap {

   RemoteGuiListener::RemoteGuiListener(RemoteGuiClientFactory &clientFactory, uint32_t clientId)
      : _clientFactory(clientFactory), _clientId(clientId) {}

   RemoteGuiListener::~RemoteGuiListener() = default;
} // namespace clap