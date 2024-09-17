#pragma once

#include "abstract-gui-listener.hh"

namespace clap {
   class ThreadedGuiListenerProxy final : public AbstractGuiListener {
   public:
      ThreadedGuiListenerProxy(AbstractGuiListener &guiListener);

      void onGuiRunOnMainThread(std::function<void()> callback) override;

      void onGuiPoll() override;

      void onGuiParamBeginAdjust(clap_id paramId) override;
      void onGuiParamAdjust(clap_id paramId, double value) override;
      void onGuiParamEndAdjust(clap_id paramId) override;

      void onGuiSetTransportIsSubscribed(bool isSubscribed) override;

      void onGuiWindowClosed(bool wasDestroyed) override;

      void onGuiUndo() override;
      void onGuiRedo() override;

      void onGuiInvoke(const std::string &method, const InvocationArgumentsType &args) override;

   private:
      AbstractGuiListener &_guiListener;
   };
} // namespace clap