#include <stdexcept>

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QThread>
#include <QWindow>

#include "../io/messages.hh"
#include "abstract-gui-listener.hh"
#include "gui.hh"

#ifdef _WIN32
#   include <windows.h>
#endif

namespace clap {

   Gui::Gui(AbstractGuiListener &listener) : AbstractGui(listener), _quickView(new QQuickView()) {

      qDebug() << "clap-gui: created gui";

      _pluginProxy = std::make_unique<PluginProxy>(*this);
      _transportProxy = std::make_unique<TransportProxy>(*this);
      _trackInfoProxy = std::make_unique<TrackInfoProxy>(*this);
      _undoProxy = std::make_unique<UndoProxy>(*this);

      ////////////////////////
      // QML initialization //
      ////////////////////////

      auto qmlContext = _quickView->engine()->rootContext();

      qmlContext->setContextProperty("plugin", _pluginProxy.get());
      qmlContext->setContextProperty("transport", _transportProxy.get());
      qmlContext->setContextProperty("trackInfo", _trackInfoProxy.get());
      qmlContext->setContextProperty("undo", _undoProxy.get());
      setRootScale(1);

      connect(
         _quickView.get(), &QQuickView::visibleChanged, this, &Gui::onQuickViewVisibilityChange);
   }

   Gui::~Gui() { destroy(); }

   void Gui::addImportPath(const std::string &importPath) {
      qDebug() << "clap-gui: addImportPath(" << importPath.c_str() << ")";
      _quickView->engine()->addImportPath(QString::fromStdString(importPath));
   }

   void Gui::setSkin(const std::string &skinUrl) {
      qDebug() << "clap-gui: skinUrl(" << skinUrl.c_str() << ")";
      _quickView->setSource(QUrl(skinUrl.c_str()));
   }

   void Gui::setGuiProperty(const std::string &name, const PropertyValue &value)
   {
      _pluginProxy->setGuiProperty(name, value);
   }

   void Gui::defineParameter(const clap_param_info &paramInfo) {
      qDebug() << "clap-gui: defineParam(" << paramInfo.id << ")";
      _pluginProxy->defineParameter(paramInfo);
   }

   void Gui::updateParameter(clap_id paramId, double value, double modAmount) {
      qDebug() << "clap-gui: updateParam(" << paramId << ")";
      auto p = _pluginProxy->param(paramId);
      assert(p);
      if (!p)
         return;

      p->setValueFromPlugin(value);
      p->setModulationFromPlugin(modAmount);
   }

   void Gui::setParameterMappingIndication(clap_id paramId,
                                           bool hasIndication,
                                           clap_color color,
                                           const char *label,
                                           const char *description) {
      qDebug() << "clap-gui: setParameterMappingIndication(" << paramId << ")";
      auto p = _pluginProxy->param(paramId);
      assert(p);
      if (!p)
         return;

      p->setMappingIndication(color, label, description);
   }

   void Gui::setParameterAutomationIndication(clap_id paramId,
                                              uint32_t automationState,
                                              clap_color color) {
      qDebug() << "clap-gui: setParameterAutomationIndication(" << paramId << ")";
      auto p = _pluginProxy->param(paramId);
      assert(p);
      if (!p)
         return;

      p->setAutomationIndication(automationState, color);
   }

   void Gui::clearTransport() { _transportProxy->clear(); }

   void Gui::updateTransport(const clap_event_transport &transport) {
      _transportProxy->update(transport);
   }

   void Gui::updateTrackInfo(bool hasTrackInfo, const clap_track_info &info) {
      _trackInfoProxy->update(hasTrackInfo, info);
   }

   void Gui::showLater() {
      QMetaObject::invokeMethod(
         this,
         [this] {
            if (_hostWindow && _quickView)
               _quickView->show();
         },
         Qt::QueuedConnection);
   }

   bool Gui::openWindow() {
      assert(!_hostWindow);
      _quickView->show();
      _isFloating = true;
      return true;
   }

   bool Gui::attachCocoa(clap_nsview nsView) {
      assert(!_isFloating);
#ifdef Q_OS_MACOS
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(nsView)));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         _quickView->show();
         return true;
      }
#endif
      return false;
   }

   bool Gui::attachWin32(clap_hwnd window) {
      assert(!_isFloating);
#ifdef Q_OS_WIN
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(window)));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         showLater();
         return true;
      }
#endif
      return false;
   }

   bool Gui::attachX11(clap_xwnd window) {
      assert(!_isFloating);
#ifdef Q_OS_LINUX
      qDebug() << "clap-gui: attachX11(" << window << ")";
      _hostWindow.reset(QWindow::fromWinId(window));
      if (_hostWindow) {
         _quickView->setParent(_hostWindow.get());
         _quickView->show();
         qGuiApp->sync();
         return true;
      }
#endif
      return false;
   }

   bool Gui::setTransientCocoa(clap_nsview nsView) {
      assert(_isFloating);
#ifdef Q_OS_MACOS
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(nsView)));
      if (_hostWindow) {
         _quickView->setTransientParent(_hostWindow.get());
         return true;
      }
#endif
      return false;
   }

   bool Gui::setTransientWin32(clap_hwnd window) {
      assert(!_isFloating);
#ifdef Q_OS_WIN
      _hostWindow.reset(QWindow::fromWinId(reinterpret_cast<WId>(window)));
      if (_hostWindow) {
         _quickView->setTransientParent(_hostWindow.get());
         return true;
      }
#endif
      return false;
   }

   bool Gui::setTransientX11(clap_xwnd window) {
#ifdef Q_OS_LINUX
      qDebug() << "clap-gui: setTransientX11(" << window << ")";
      _hostWindow.reset(QWindow::fromWinId(window));
      if (_hostWindow) {
         _quickView->setTransientParent(_hostWindow.get());
         return true;
      }
#endif
      return false;
   }

   bool Gui::wantsLogicalSize() noexcept {
#ifdef Q_OS_MACOS
      return true;
#else
      return false;
#endif
   }

   bool Gui::canResize() { return true; }

   bool Gui::getSize(uint32_t *width, uint32_t *height) {
      if (!_quickView)
         return false;

      while (_quickView->status() == QQuickView::Status::Loading) {
         QThread::sleep(10);
         qGuiApp->processEvents();
      }

      auto rootItem = _quickView->rootObject();
      if (!rootItem)
         return false;

      auto ratio = wantsLogicalSize() ? 1 : _quickView->devicePixelRatio();
      *width = std::ceil(rootItem->width() * ratio);
      *height = std::ceil(rootItem->height() * ratio);
      return true;
   }

   bool Gui::setSize(uint32_t width, uint32_t height) {
      auto root = _quickView->rootObject();
      if (!root)
         return false;

      if (!wantsLogicalSize()) {
         auto ratio = _quickView->devicePixelRatio();
         width /= ratio;
         height /= ratio;
      }

      _quickView->setWidth(width);
      _quickView->setHeight(height);

      double rw = _rootScale * double(width) / root->width();
      double rh = _rootScale * double(height) / root->height();
      double scale = std::min<double>(rw, rh);
      root->setTransformOrigin(QQuickItem::TopLeft);
      setRootScale(scale);

      return true;
   }

   void Gui::setRootScale(double scale) {
      _rootScale = scale;
      _quickView->engine()->rootContext()->setContextProperty("rootScale", scale);
   }

   bool Gui::roundSize(uint32_t *width, uint32_t *height) {
      uint32_t w, h;
      if (!getSize(&w, &h))
         return false;

      double originalWidth = w / _rootScale;
      double originalHeight = h / _rootScale;

      if (*width < originalWidth)
         *width = originalWidth;
      if (*height < originalHeight)
         *height = originalHeight;

      double rw = double(*width) / double(w);
      double rh = double(*height) / double(h);
      double r = std::min<double>(rw, rh);

      uint32_t finalWidth = std::ceil(r * w);
      uint32_t finalHeight = std::ceil(r * h);

      // we want the value of roundSize to be stable
      if (finalWidth != *width || finalHeight != *height)
         roundSize(&finalWidth, &finalHeight);

      *width = finalWidth;
      *height = finalHeight;
      return true;
   }

   bool Gui::setScale(double scale) { return false; }

   bool Gui::show() {
      _quickView->show();
      return true;
   }

   bool Gui::hide() {
      _quickView->hide();
      return true;
   }

   void Gui::onQuickViewVisibilityChange(bool isVisible) {
      if (!isVisible && _isFloating)
         _listener.onGuiWindowClosed(false);
   }

   void Gui::destroy() {
      qDebug() << "clap-gui: destroy()";
      _quickView.reset();
      _hostWindow.reset();
      _transportProxy.reset();
      _pluginProxy.reset();
      _isFloating = false;
   }

   void Gui::setCanUndo(bool can_undo) { _undoProxy->setCanUndo(can_undo); }

   void Gui::setCanRedo(bool can_redo) { _undoProxy->setCanRedo(can_redo); }

   void Gui::setUndoName(std::string name) {
      _undoProxy->setUndoName(QString::fromStdString(name));
   }

   void Gui::setRedoName(std::string name) {
      _undoProxy->setRedoName(QString::fromStdString(name));
   }
} // namespace clap