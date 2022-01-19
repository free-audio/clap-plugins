#include <exception>

#include "gui-client.hh"
#include "local-gui-proxy.hh"

namespace clap {

   LocalGuiProxy::LocalGuiProxy(AbstractGuiListener &listener, GuiClient &guiClient)
      : AbstractGui(listener), _guiClient(guiClient) {}

   LocalGuiProxy::~LocalGuiProxy() {}

   void clap::LocalGuiProxy::defineParameter(const clap_param_info &paramInfo) {
      QMetaObject::invokeMethod(
         &_guiClient, [=, this] { _guiClient.defineParameter(paramInfo); }, Qt::QueuedConnection);
   }

   void LocalGuiProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      QMetaObject::invokeMethod(
         &_guiClient,
         [=, this] { _guiClient.updateParameter(paramId, value, modAmount); },
         Qt::QueuedConnection);
   }

   void LocalGuiProxy::clearTransport() {
      QMetaObject::invokeMethod(
         &_guiClient, [=, this] { _guiClient.clearTransport(); }, Qt::QueuedConnection);
   }

   void LocalGuiProxy::updateTransport(const clap_event_transport &transport) {
      QMetaObject::invokeMethod(
         &_guiClient, [=, this] { _guiClient.updateTransport(transport); }, Qt::QueuedConnection);
   }

   bool LocalGuiProxy::attachCocoa(void *nsView) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient,
         [=, this, &succeed] { succeed = _guiClient.attachCocoa(nsView); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool LocalGuiProxy::attachWin32(clap_hwnd window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient,
         [=, this, &succeed] { succeed = _guiClient.attachWin32(window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool LocalGuiProxy::attachX11(const char *displayName, unsigned long window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient,
         [=, this, &succeed] { succeed = _guiClient.attachX11(displayName, window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool LocalGuiProxy::size(uint32_t *width, uint32_t *height) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient,
         [=, this, &succeed] { succeed = _guiClient.size(width, height); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool LocalGuiProxy::setScale(double scale) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient,
         [=, this, &succeed] { succeed = _guiClient.setScale(scale); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool LocalGuiProxy::show() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient, [=, this, &succeed] { succeed = _guiClient.show(); }, Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool LocalGuiProxy::hide() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         &_guiClient, [=, this, &succeed] { succeed = _guiClient.hide(); }, Qt::BlockingQueuedConnection);
      return succeed;
   }

   void LocalGuiProxy::destroy() {
      // todo
   }
} // namespace clap
