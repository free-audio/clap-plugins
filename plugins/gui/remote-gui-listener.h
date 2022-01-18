#include <memory>

#include "abstract-gui-listener.h"

namespace clap {

   class RemoteChannel;
   class RemoteGuiListener : public AbstractGuiListener {
   public:
      RemoteGuiListener(int socket);
      RemoteGuiListener(void *pipeIn, void *pipeOut);
      ~RemoteGuiListener() override;

      virtual void onGuiPoll() = 0;

      virtual void onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) = 0;
      virtual void onGuiSetTransportIsSubscribed(bool isSubscribed) = 0;

   protected:
      std::unique_ptr<clap::RemoteChannel> _remoteChannel;
   };

} // namespace clap