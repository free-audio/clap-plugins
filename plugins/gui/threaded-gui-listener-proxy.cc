#include "threaded-gui-listener-proxy.hh"

namespace clap {
   ThreadedGuiListenerProxy::ThreadedGuiListenerProxy(AbstractGuiListener &guiListener)
      : _guiListener(guiListener) {}

   void ThreadedGuiListenerProxy::onGuiRunOnMainThread(std::function<void()> callback) {
      _guiListener.onGuiRunOnMainThread(std::move(callback));
   }

   void ThreadedGuiListenerProxy::onGuiPoll() {
      onGuiRunOnMainThread([this] { _guiListener.onGuiPoll(); });
   }

   void ThreadedGuiListenerProxy::onGuiParamBeginAdjust(clap_id paramId) {
      onGuiRunOnMainThread([this, paramId] { _guiListener.onGuiParamBeginAdjust(paramId); });
   }

   void ThreadedGuiListenerProxy::onGuiParamAdjust(clap_id paramId, double value) {
      onGuiRunOnMainThread([this, paramId, value] { _guiListener.onGuiParamAdjust(paramId, value); });
   }

   void ThreadedGuiListenerProxy::onGuiParamEndAdjust(clap_id paramId) {
      onGuiRunOnMainThread([this, paramId] { _guiListener.onGuiParamEndAdjust(paramId); });
   }

   void ThreadedGuiListenerProxy::onGuiSetTransportIsSubscribed(bool isSubscribed) {
      onGuiRunOnMainThread(
         [this, isSubscribed] { _guiListener.onGuiSetTransportIsSubscribed(isSubscribed); });
   }

   void ThreadedGuiListenerProxy::onGuiWindowClosed(bool wasDestroyed) {
      onGuiRunOnMainThread([this, wasDestroyed] { _guiListener.onGuiWindowClosed(wasDestroyed); });
   }

   void ThreadedGuiListenerProxy::onGuiUndo() {
      onGuiRunOnMainThread([this] { _guiListener.onGuiUndo(); });
   }

   void ThreadedGuiListenerProxy::onGuiRedo() {
      onGuiRunOnMainThread([this] { _guiListener.onGuiRedo(); });
   }

   void ThreadedGuiListenerProxy::onGuiInvoke(const std::string &method,
                                              const InvocationArgumentsType &args) {
      onGuiRunOnMainThread([this, method, args] { _guiListener.onGuiInvoke(method, args); });
   }
} // namespace clap