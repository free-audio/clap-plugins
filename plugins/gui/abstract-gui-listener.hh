#pragma once

#include <clap/clap.h>

namespace clap {
   // Listener for events produced by the Gui, to the destination of the plugin.
   //
   // Beaware that the callbacks happen on the plugin's GUI thread, not the host's main thread.
   class AbstractGuiListener {
   public:
      virtual ~AbstractGuiListener();

      // timer based polling of new parameter changes, transport value, ...
      // the plugin shall transmit them via
      virtual void onGuiPoll() = 0;

      virtual void onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) = 0;
      virtual void onGuiSetTransportIsSubscribed(bool isSubscribed) = 0;

      virtual void onGuiWindowClosed(bool wasDestroyed) = 0;
   };
} // namespace clap