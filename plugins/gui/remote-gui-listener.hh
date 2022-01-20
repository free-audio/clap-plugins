#include <memory>

#include "abstract-gui-listener.hh"

namespace clap {

   class RemoteChannel;
   class RemoteGuiListener : public AbstractGuiListener {
   public:
      RemoteGuiListener(int socket);
      RemoteGuiListener(void *pipeIn, void *pipeOut);
      ~RemoteGuiListener() override;

      virtual void onGuiPoll() override;

      virtual void onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) override;
      void onGuiSetTransportIsSubscribed(bool isSubscribed) override;

   protected:
      std::unique_ptr<clap::RemoteChannel> _remoteChannel;
   };

} // namespace clap