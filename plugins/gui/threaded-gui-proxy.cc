#include <exception>

#include "gui-client.hh"
#include "threaded-gui-proxy.hh"

namespace clap {

   ThreadedGuiProxy::ThreadedGuiProxy(AbstractGuiListener &listener, std::shared_ptr<GuiClient>& guiClient)
      : AbstractGui(listener), _guiClient(guiClient) {}

   ThreadedGuiProxy::~ThreadedGuiProxy() {}

   void ThreadedGuiProxy::setSkin(const std::string &skinUrl)
   {
      QMetaObject::invokeMethod(
         _guiClient.get(), [=, this] { _guiClient->setSkin(skinUrl); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::defineParameter(const clap_param_info &paramInfo) {
      QMetaObject::invokeMethod(
         _guiClient.get(), [=, this] { _guiClient->defineParameter(paramInfo); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      QMetaObject::invokeMethod(
         _guiClient.get(),
         [=, this] { _guiClient->updateParameter(paramId, value, modAmount); },
         Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::clearTransport() {
      QMetaObject::invokeMethod(
         _guiClient.get(), [=, this] { _guiClient->clearTransport(); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::updateTransport(const clap_event_transport &transport) {
      QMetaObject::invokeMethod(
         _guiClient.get(), [=, this] { _guiClient->updateTransport(transport); }, Qt::QueuedConnection);
   }

   bool ThreadedGuiProxy::attachCocoa(void *nsView) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(),
         [=, this, &succeed] { succeed = _guiClient->attachCocoa(nsView); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::attachWin32(clap_hwnd window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(),
         [=, this, &succeed] { succeed = _guiClient->attachWin32(window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::attachX11(const char *displayName, unsigned long window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(),
         [=, this, &succeed] { succeed = _guiClient->attachX11(displayName, window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::size(uint32_t *width, uint32_t *height) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(),
         [=, this, &succeed] { succeed = _guiClient->size(width, height); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::setScale(double scale) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(),
         [=, this, &succeed] { succeed = _guiClient->setScale(scale); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::show() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(), [=, this, &succeed] { succeed = _guiClient->show(); }, Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::hide() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _guiClient.get(), [=, this, &succeed] { succeed = _guiClient->hide(); }, Qt::BlockingQueuedConnection);
      return succeed;
   }

   void ThreadedGuiProxy::destroy() {
      // todo
   }
} // namespace clap
