#include <algorithm>
#include <cmath>

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include "cocoa-window.hh"

// Compiled with ARC

namespace clap {

   namespace {
      constexpr CFTimeInterval kTimerPeriod = 1. / 60;

      // Scroll distance in points corresponding to one line, for the trackpads
      constexpr double kScrollDistancePerLine = 10;

      // The view keeps a pointer to its CocoaWindow, under this key
      char kWindowKey;

      CocoaWindow *windowOf(id view) {
         NSValue *value = objc_getAssociatedObject(view, &kWindowKey);
         return static_cast<CocoaWindow *>(value.pointerValue);
      }

      // The view is flipped: the origin is the top left corner
      NSPoint location(id view, NSEvent *event) {
         return [(NSView *)view convertPoint:event.locationInWindow fromView:nil];
      }

      ////////////////////
      // View's methods //
      ////////////////////
      BOOL returnYes(id, SEL) { return YES; }

      BOOL acceptsFirstMouse(id, SEL, NSEvent *) { return YES; }

      void drawRect(id self, SEL, NSRect) {
         if (auto *window = windowOf(self))
            window->draw([NSGraphicsContext currentContext].CGContext);
      }

      void mouseDragged(id self, SEL, NSEvent *event) {
         if (auto *window = windowOf(self)) {
            const NSPoint p = location(self, event);
            window->onMouseMove(p.x, p.y, false);
         }
      }

      // Also for mouseEntered:, which may come during a drag
      void mouseMoved(id self, SEL, NSEvent *event) {
         if (auto *window = windowOf(self)) {
            const NSPoint p = location(self, event);
            window->onMouseMove(p.x, p.y, NSEvent.pressedMouseButtons == 0);
         }
      }

      void mouseExited(id self, SEL, NSEvent *) {
         if (auto *window = windowOf(self))
            window->onMouseExited();
      }

      template <int index, bool isPressed>
      void mouseButton(id self, SEL, NSEvent *event) {
         if (auto *window = windowOf(self)) {
            const NSPoint p = location(self, event);
            window->onMouseButton(index, isPressed, p.x, p.y);
         }
      }

      // Only the middle button
      template <bool isPressed>
      void otherMouseButton(id self, SEL _cmd, NSEvent *event) {
         if (event.buttonNumber == 2)
            mouseButton<2, isPressed>(self, _cmd, event);
      }

      void scrollWheel(id self, SEL, NSEvent *event) {
         auto *window = windowOf(self);
         if (!window)
            return;

         double dx = event.scrollingDeltaX;
         double dy = event.scrollingDeltaY;
         if (event.hasPreciseScrollingDeltas) {
            dx /= kScrollDistancePerLine;
            dy /= kScrollDistancePerLine;
         }
         window->onScroll(dx, dy);
      }

      void backingChanged(id self, SEL) {
         if (auto *window = windowOf(self))
            window->updateScale();
      }

      // Several plugins may define their own view class, so the name is unique to this copy of
      // the plugin.
      Class viewClass() {
         static Class cls = []() -> Class {
            NSString *name =
               [NSString stringWithFormat:@"ClapPluginsImGuiView_%p", (void *)&kWindowKey];
            Class c = objc_allocateClassPair([NSView class], name.UTF8String, 0);
            if (!c)
               return objc_getClass(name.UTF8String);

            // They all override NSView's methods, with the same signature
            auto add = [c](SEL selector, IMP imp) {
               Method method = class_getInstanceMethod([NSView class], selector);
               class_addMethod(c, selector, imp, method_getTypeEncoding(method));
            };
            add(@selector(isFlipped), (IMP)&returnYes);
            add(@selector(isOpaque), (IMP)&returnYes);
            add(@selector(acceptsFirstMouse:), (IMP)&acceptsFirstMouse);
            add(@selector(drawRect:), (IMP)&drawRect);
            add(@selector(mouseMoved:), (IMP)&mouseMoved);
            add(@selector(mouseEntered:), (IMP)&mouseMoved);
            add(@selector(mouseDragged:), (IMP)&mouseDragged);
            add(@selector(rightMouseDragged:), (IMP)&mouseDragged);
            add(@selector(otherMouseDragged:), (IMP)&mouseDragged);
            add(@selector(mouseExited:), (IMP)&mouseExited);
            add(@selector(mouseDown:), (IMP)&mouseButton<0, true>);
            add(@selector(mouseUp:), (IMP)&mouseButton<0, false>);
            add(@selector(rightMouseDown:), (IMP)&mouseButton<1, true>);
            add(@selector(rightMouseUp:), (IMP)&mouseButton<1, false>);
            add(@selector(otherMouseDown:), (IMP)&otherMouseButton<true>);
            add(@selector(otherMouseUp:), (IMP)&otherMouseButton<false>);
            add(@selector(scrollWheel:), (IMP)&scrollWheel);
            add(@selector(viewDidChangeBackingProperties), (IMP)&backingChanged);
            add(@selector(viewDidMoveToWindow), (IMP)&backingChanged);
            objc_registerClassPair(c);
            return c;
         }();
         return cls;
      }

      void timerFired(CFRunLoopTimerRef, void *info) {
         @autoreleasepool {
            static_cast<CocoaWindow *>(info)->onTimerFired();
         }
      }

      NSView *viewOf(void *view) { return (__bridge NSView *)view; }
      NSWindow *windowOf(void *window) { return (__bridge NSWindow *)window; }
   } // namespace

   CocoaWindow::CocoaWindow(Listener &listener) : PlatformWindow(listener) {}

   CocoaWindow::~CocoaWindow() { destroy(); }

   bool CocoaWindow::open(void *parent) {
      NSView *parentView = (__bridge NSView *)parent;
      _isFloating = !parentView;

      NSView *view = [[viewClass() alloc] initWithFrame:NSMakeRect(0, 0, _width, _height)];
      objc_setAssociatedObject(
         view, &kWindowKey, [NSValue valueWithPointer:this], OBJC_ASSOCIATION_RETAIN_NONATOMIC);

      // It follows the visible part of the view, so we don't have to update it
      NSTrackingArea *area = [[NSTrackingArea alloc]
         initWithRect:NSZeroRect
              options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                      NSTrackingActiveAlways | NSTrackingInVisibleRect
                owner:view
             userInfo:nil];
      [view addTrackingArea:area];

      if (_isFloating)
         openWindow((__bridge void *)view);
      else {
         view.hidden = YES;
         [parentView addSubview:view];
      }
      _view = (__bridge_retained void *)view;
      updateScale();

      // The common modes keep it firing during the modal loops: resizing a window, menus, ...
      CFRunLoopTimerContext context = {0, this, nullptr, nullptr, nullptr};
      CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault,
                                                     CFAbsoluteTimeGetCurrent() + kTimerPeriod,
                                                     kTimerPeriod,
                                                     0,
                                                     0,
                                                     timerFired,
                                                     &context);
      CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
      _timer = timer;
      return true;
   }

   // A floating window, whose content is the view
   void CocoaWindow::openWindow(void *contentView) {
      NSView *view = viewOf(contentView);
      NSWindow *window = [[NSWindow alloc]
         initWithContentRect:NSMakeRect(0, 0, _width, _height)
                   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                     backing:NSBackingStoreBuffered
                       defer:YES];
      // Closing it only hides it
      window.releasedWhenClosed = NO;
      window.contentView = view;
      window.contentMinSize = NSMakeSize(_minWidth, _minHeight);
      // nil for invalid UTF-8, which the title doesn't accept
      window.title = [NSString stringWithUTF8String:_title.c_str()] ?: @"";
      [window center];

      auto *cocoaWindow = this;
      NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
      id closeObserver = [center addObserverForName:NSWindowWillCloseNotification
                                             object:window
                                              queue:nil
                                         usingBlock:^(NSNotification *notification) {
                                           cocoaWindow->onWindowClosed();
                                         }];
      id resizeObserver = [center addObserverForName:NSWindowDidResizeNotification
                                              object:window
                                               queue:nil
                                          usingBlock:^(NSNotification *notification) {
                                            cocoaWindow->onWindowResized();
                                          }];
      _observers = (__bridge_retained void *)@[closeObserver, resizeObserver];
      _window = (__bridge_retained void *)window;
   }

   void CocoaWindow::destroy() {
      if (_timer) {
         auto timer = static_cast<CFRunLoopTimerRef>(_timer);
         CFRunLoopTimerInvalidate(timer);
         CFRelease(timer);
         _timer = nullptr;
      }

      if (_observers) {
         NSArray *observers = (__bridge_transfer NSArray *)_observers;
         _observers = nullptr;
         for (id observer in observers)
            [NSNotificationCenter.defaultCenter removeObserver:observer];
      }

      if (_window) {
         NSWindow *window = (__bridge_transfer NSWindow *)_window;
         _window = nullptr;
         [window.parentWindow removeChildWindow:window];
         [window orderOut:nil];
         window.contentView = nil;
      }

      if (_transientParent) {
         CFRelease(_transientParent);
         _transientParent = nullptr;
      }

      if (_view) {
         NSView *view = (__bridge_transfer NSView *)_view;
         _view = nullptr;

         // The view may outlive us if the host retains it
         objc_setAssociatedObject(view, &kWindowKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
         [view removeFromSuperview];
      }
   }

   void CocoaWindow::setSize(uint32_t width, uint32_t height) {
      _width = std::max(width, _minWidth);
      _height = std::max(height, _minHeight);
      if (_window)
         [windowOf(_window) setContentSize:NSMakeSize(_width, _height)];
      else if (_view)
         [viewOf(_view) setFrameSize:NSMakeSize(_width, _height)];
   }

   void CocoaWindow::setMinSize(uint32_t width, uint32_t height) {
      _minWidth = std::max(width, 1u);
      _minHeight = std::max(height, 1u);
      if (_window)
         windowOf(_window).contentMinSize = NSMakeSize(_minWidth, _minHeight);
   }

   void CocoaWindow::setTitle(const std::string &title) {
      _title = title;
      if (_window)
         windowOf(_window).title = [NSString stringWithUTF8String:title.c_str()] ?: @"";
   }

   // The child window stays above its parent, and moves with it
   bool CocoaWindow::setTransient(void *hostView) {
      if (!_window)
         return false;

      NSWindow *parent = ((__bridge NSView *)hostView).window;
      if (!parent)
         return false;

      if (_transientParent)
         CFRelease(_transientParent);
      _transientParent = (__bridge_retained void *)parent;

      NSWindow *window = windowOf(_window);
      if (window.isVisible)
         [parent addChildWindow:window ordered:NSWindowAbove];
      return true;
   }

   void CocoaWindow::setVisible(bool isVisible) {
      if (!_view)
         return;

      if (!isVisible)
         leavePointer();

      if (!_window) {
         viewOf(_view).hidden = !isVisible;
         return;
      }

      NSWindow *window = windowOf(_window);
      if (!isVisible) {
         [window orderOut:nil];
         return;
      }

      // Hiding a child window detaches it from its parent
      if (_transientParent && window.parentWindow != windowOf(_transientParent))
         [windowOf(_transientParent) addChildWindow:window ordered:NSWindowAbove];
      [window makeKeyAndOrderFront:nil];
   }

   // The user closed the floating window, which is now hidden
   void CocoaWindow::onWindowClosed() {
      leavePointer();
      _listener.onCloseRequested();
   }

   // The user resized the floating window
   void CocoaWindow::onWindowResized() {
      if (!_view)
         return;

      const NSSize size = viewOf(_view).bounds.size;
      const uint32_t width = std::max<uint32_t>(1, std::lround(size.width));
      const uint32_t height = std::max<uint32_t>(1, std::lround(size.height));
      if (width == _width && height == _height)
         return;

      _width = width;
      _height = height;
      _listener.onResized(width, height);
   }

   void CocoaWindow::updateScale() {
      if (!_view)
         return;

      NSWindow *window = viewOf(_view).window;
      const double scale =
         window ? window.backingScaleFactor : NSScreen.mainScreen.backingScaleFactor;
      if (scale <= 0 || scale == _scale)
         return;

      _scale = scale;
      _listener.onScaleChanged();
   }

   ////////////
   // Frames //
   ////////////
   Framebuffer *CocoaWindow::beginFrame() {
      if (!_view)
         return nullptr;

      NSView *view = viewOf(_view);
      if (!view.window || !view.window.isVisible || view.isHiddenOrHasHiddenAncestor)
         return nullptr;

      const int width = std::max<int>(1, std::lround(_width * _scale));
      const int height = std::max<int>(1, std::lround(_height * _scale));
      _pixels.resize(size_t(width) * height);
      _framebuffer.pixels = _pixels.data();
      _framebuffer.width = width;
      _framebuffer.height = height;
      _framebuffer.stride = width;
      return &_framebuffer;
   }

   // The view draws the frame when AppKit displays it
   void CocoaWindow::endFrame() {
      _hasFrame = true;
      [viewOf(_view) setNeedsDisplay:YES];
   }

   void CocoaWindow::draw(void *cgContext) {
      auto context = static_cast<CGContextRef>(cgContext);
      if (!context)
         return;

      // The view is opaque
      if (!_hasFrame) {
         CGContextSetGrayFillColor(context, 0, 1);
         CGContextFillRect(context, CGRectMake(0, 0, _width, _height));
         return;
      }

      const auto &fb = _framebuffer;
      CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
      CGDataProviderRef provider = CGDataProviderCreateWithData(
         nullptr, fb.pixels, size_t(fb.stride) * fb.height * 4, nullptr);
      // Our pixels are XRGB, in native 32 bits words
      CGImageRef image = CGImageCreate(
         fb.width,
         fb.height,
         8,
         32,
         size_t(fb.stride) * 4,
         colorSpace,
         static_cast<CGBitmapInfo>(uint32_t(kCGImageAlphaNoneSkipFirst) |
                                   uint32_t(kCGBitmapByteOrder32Little)),
         provider,
         nullptr,
         false,
         kCGRenderingIntentDefault);

      // The image would be upside down in the flipped view
      CGContextSaveGState(context);
      CGContextTranslateCTM(context, 0, _height);
      CGContextScaleCTM(context, 1, -1);
      CGContextSetInterpolationQuality(context, kCGInterpolationNone);
      CGContextDrawImage(context, CGRectMake(0, 0, _width, _height), image);
      CGContextRestoreGState(context);

      CGImageRelease(image);
      CGDataProviderRelease(provider);
      CGColorSpaceRelease(colorSpace);
   }

   ///////////
   // Input //
   ///////////
   void CocoaWindow::onMouseMove(double x, double y, bool hasNoButton) {
      // We missed the release, for example because of a system gesture
      if (hasNoButton && _pressedButtons) {
         for (int i = 0; i < 3; ++i)
            if (_pressedButtons & (1u << i))
               _listener.onPointerButton(i, false);
         _pressedButtons = 0;
         _hasLeftWhilePressed = false;
      }

      // Back in the view during a drag
      if (_hasLeftWhilePressed && x >= 0 && y >= 0 && x < _width && y < _height)
         _hasLeftWhilePressed = false;

      _isPointerInside = true;
      _listener.onPointerMove(x, y);
   }

   void CocoaWindow::onMouseExited() {
      // During a drag, we keep getting the moves outside of the view
      if (_pressedButtons)
         _hasLeftWhilePressed = true;
      else
         leavePointer();
   }

   void CocoaWindow::onMouseButton(int index, bool isPressed, double x, double y) {
      onMouseMove(x, y, false);

      if (isPressed)
         _pressedButtons |= 1u << index;
      else
         _pressedButtons &= ~(1u << index);
      _listener.onPointerButton(index, isPressed);

      if (!_pressedButtons && _hasLeftWhilePressed)
         leavePointer();
   }

   void CocoaWindow::onScroll(double dx, double dy) { _listener.onPointerScroll(dx, dy); }

   void CocoaWindow::leavePointer() {
      _hasLeftWhilePressed = false;
      if (!_isPointerInside)
         return;

      for (int i = 0; i < 3; ++i)
         if (_pressedButtons & (1u << i))
            _listener.onPointerButton(i, false);
      _pressedButtons = 0;

      _isPointerInside = false;
      _listener.onPointerLeave();
   }

} // namespace clap
