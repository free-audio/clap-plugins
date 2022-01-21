#pragma once

#include <memory>

#include "../io/remote-channel.hh"

#include "abstract-gui-listener.hh"

namespace clap {
   class RemoteGuiFactory;
   class RemoteGuiListener : public AbstractGuiListener {
   public:
      RemoteGuiListener(RemoteGuiFactory& clientFactory, uint32_t clientId);
      ~RemoteGuiListener() override;

      void onGuiPoll() override;

      void onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) override;
      void onGuiSetTransportIsSubscribed(bool isSubscribed) override;

   protected:
      friend class RemoteGuiFactory;

      RemoteGuiFactory& _clientFactory;
      const uint32_t _clientId;
   };

} // namespace clap