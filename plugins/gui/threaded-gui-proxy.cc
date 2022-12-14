#include <exception>

#include "gui.hh"
#include "threaded-gui-proxy.hh"

namespace clap {

   ThreadedGuiProxy::ThreadedGuiProxy(AbstractGuiListener &listener,
                                      std::shared_ptr<Gui> &guiClient)
      : AbstractGui(listener), _gui(guiClient) {}

   ThreadedGuiProxy::~ThreadedGuiProxy() {}

   void ThreadedGuiProxy::addImportPath(const std::string &importPath) {
      QMetaObject::invokeMethod(
         _gui.get(), [=, this] { _gui->addImportPath(importPath); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::setSkin(const std::string &skinUrl) {
      QMetaObject::invokeMethod(
         _gui.get(), [=, this] { _gui->setSkin(skinUrl); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::defineParameter(const clap_param_info &paramInfo) {
      QMetaObject::invokeMethod(
         _gui.get(), [=, this] { _gui->defineParameter(paramInfo); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::updateParameter(clap_id paramId, double value, double modAmount) {
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this] { _gui->updateParameter(paramId, value, modAmount); },
         Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::setParameterMappingIndication(clap_id paramId,
                                                        bool hasIndication,
                                                        clap_color color,
                                                        const char *label,
                                                        const char *description) {
      QMetaObject::invokeMethod(
         const std::string _label(label);
         const std::string _description(description) _gui.get(),
         [this, paramId, hasIndication, color, _label, _description] {
            _gui->setParameterMappingIndication(
               paramId, hasIndication, color, _label.c_str(), _description.c_str());
         },
         Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::clearTransport() {
      QMetaObject::invokeMethod(
         _gui.get(), [=, this] { _gui->clearTransport(); }, Qt::QueuedConnection);
   }

   void ThreadedGuiProxy::updateTransport(const clap_event_transport &transport) {
      QMetaObject::invokeMethod(
         _gui.get(), [=, this] { _gui->updateTransport(transport); }, Qt::QueuedConnection);
   }

   bool ThreadedGuiProxy::openWindow() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->openWindow(); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::attachCocoa(clap_nsview nsView) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->attachCocoa(nsView); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::attachWin32(clap_hwnd window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->attachWin32(window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::attachX11(clap_xwnd window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->attachX11(window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::setTransientCocoa(clap_nsview nsView) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->setTransientCocoa(nsView); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::setTransientWin32(clap_hwnd window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->setTransientWin32(window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::setTransientX11(clap_xwnd window) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->setTransientX11(window); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::canResize() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->canResize(); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::getSize(uint32_t *width, uint32_t *height) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->getSize(width, height); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::setSize(uint32_t width, uint32_t height) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->setSize(width, height); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::roundSize(uint32_t *width, uint32_t *height) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->roundSize(width, height); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::setScale(double scale) {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(),
         [=, this, &succeed] { succeed = _gui->setScale(scale); },
         Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::show() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(), [=, this, &succeed] { succeed = _gui->show(); }, Qt::BlockingQueuedConnection);
      return succeed;
   }

   bool ThreadedGuiProxy::hide() {
      bool succeed = false;
      QMetaObject::invokeMethod(
         _gui.get(), [=, this, &succeed] { succeed = _gui->hide(); }, Qt::BlockingQueuedConnection);
      return succeed;
   }

   void ThreadedGuiProxy::destroy() {
      QMetaObject::invokeMethod(
         _gui.get(), [=, this] { _gui->destroy(); }, Qt::BlockingQueuedConnection);
   }
} // namespace clap
