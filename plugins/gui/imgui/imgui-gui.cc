#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>

#include <imgui.h>

#include "../abstract-gui-listener.hh"
#include "imgui-gui.hh"
#include "skins.hh"
#include "software-renderer.hh"

#ifdef __linux__
#   include "wayland-window.hh"
#endif

namespace clap {

   namespace {
      // Enough for ImGui to process an input event, and update the hovered and active items
      constexpr int kFramesPerUpdate = 3;

      constexpr uint32_t kMinSize = 64;

      // For the ImGui windows which cover a fixed part of the platform window
      constexpr ImGuiWindowFlags kFixedWindowFlags =
         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

      constexpr ImU32 kTitleBarColor = IM_COL32(0x20, 0x20, 0x20, 0xff);
      constexpr ImU32 kCloseButtonHoveredColor = IM_COL32(0xc0, 0x30, 0x30, 0xff);

      // Each plugin instance has its own ImGui context
      class ContextScope {
      public:
         explicit ContextScope(ImGuiContext *context) : _previous(ImGui::GetCurrentContext()) {
            ImGui::SetCurrentContext(context);
         }
         ~ContextScope() { ImGui::SetCurrentContext(_previous); }

      private:
         ImGuiContext *const _previous;
      };
   } // namespace

   double GuiParam::clip(double v) const {
      return std::clamp(v, info.min_value, info.max_value);
   }

   double GuiParam::normalize(double v) const {
      const double range = info.max_value - info.min_value;
      return range > 0 ? (clip(v) - info.min_value) / range : 0;
   }

   double GuiParam::denormalize(double v) const {
      return clip(info.min_value + std::clamp(v, 0., 1.) * (info.max_value - info.min_value));
   }

   ImGuiGui::ImGuiGui(AbstractGuiListener &listener)
      : AbstractGui(listener), _skin(&findSkin("")) {
      _context = ImGui::CreateContext();
      ContextScope scope(_context);

      auto &io = ImGui::GetIO();
      io.IniFilename = nullptr;
      io.LogFilename = nullptr;
      io.BackendPlatformName = "clap-plugins";
      io.BackendRendererName = "clap-plugins-software";
      io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
      io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
      io.Fonts->AddFontDefaultVector();

      ImGui::StyleColorsDark();
   }

   ImGuiGui::~ImGuiGui() { destroy(); }

   void ImGuiGui::destroy() {
#ifdef __linux__
      _waylandWindow = nullptr;
#endif
      _window.reset();

      if (!_context)
         return;

      {
         ContextScope scope(_context);
         for (auto *tex : ImGui::GetPlatformIO().Textures) {
            tex->SetTexID(ImTextureID_Invalid);
            tex->SetStatus(ImTextureStatus_Destroyed);
         }
      }

      ImGui::DestroyContext(_context);
      _context = nullptr;
   }

   void ImGuiGui::setSkin(const std::string &skinName) {
      _skin = &findSkin(skinName);
      _skin->defaultSize(*this, &_width, &_height);
      if (_window)
         _window->setSize(_width, _height);
      requestRedraw();
   }

   void ImGuiGui::setGuiProperty(const std::string &name, const PropertyValue &value) {
      _properties[name] = value;
      requestRedraw();
   }

   ////////////////
   // Parameters //
   ////////////////
   GuiParam *ImGuiGui::param(clap_id paramId) {
      auto it = _params.find(paramId);
      return it != _params.end() ? &it->second : nullptr;
   }

   void ImGuiGui::defineParameter(const clap_param_info &paramInfo) {
      auto [it, isNew] = _params.try_emplace(paramInfo.id);
      auto &p = it->second;
      p.info = paramInfo;

      if (isNew) {
         p.value = paramInfo.default_value;
         _paramIds.push_back(paramInfo.id);
      } else
         p.value = p.clip(p.value);

      requestRedraw();
   }

   void ImGuiGui::updateParameter(clap_id paramId, double value, double modAmount) {
      auto p = param(paramId);
      if (!p)
         return;

      // The user has the priority
      if (!p->isAdjusting)
         p->value = p->clip(value);
      p->modulation = modAmount;
      requestRedraw();
   }

   void ImGuiGui::setParameterMappingIndication(clap_id paramId,
                                                bool hasIndication,
                                                clap_color color,
                                                const char *label,
                                                const char *description) {
      auto p = param(paramId);
      if (!p)
         return;

      p->hasMappingIndication = hasIndication;
      p->mappingIndicationColor = color;
      p->mappingIndicationLabel = label ? label : "";
      p->mappingIndicationDescription = description ? description : "";
      requestRedraw();
   }

   void ImGuiGui::setParameterAutomationIndication(clap_id paramId,
                                                   uint32_t automationState,
                                                   clap_color color) {
      auto p = param(paramId);
      if (!p)
         return;

      p->automationIndicationState = automationState;
      p->automationIndicationColor = color;
      requestRedraw();
   }

   void ImGuiGui::beginAdjust(GuiParam &param) {
      if (param.isAdjusting)
         return;

      param.isAdjusting = true;
      param.dragValue = param.normalizedValue();
      _listener.onGuiParamBeginAdjust(param.info.id);
   }

   void ImGuiGui::adjust(GuiParam &param, double value) {
      value = param.clip(value);
      if (param.info.flags & CLAP_PARAM_IS_STEPPED)
         value = std::round(value);

      if (value == param.value)
         return;

      param.value = value;
      _listener.onGuiParamAdjust(param.info.id, value);
   }

   void ImGuiGui::endAdjust(GuiParam &param) {
      if (!param.isAdjusting)
         return;

      param.isAdjusting = false;
      _listener.onGuiParamEndAdjust(param.info.id);
   }

   void ImGuiGui::setToDefault(GuiParam &param) {
      const bool wasAdjusting = param.isAdjusting;

      if (!wasAdjusting)
         beginAdjust(param);
      adjust(param, param.info.default_value);
      param.dragValue = param.normalizedValue();
      if (!wasAdjusting)
         endAdjust(param);
   }

   ////////////////////////////
   // Transport & Track Info //
   ////////////////////////////
   void ImGuiGui::clearTransport() {
      _transport.reset();
      requestRedraw();
   }

   void ImGuiGui::updateTransport(const clap_event_transport &transport) {
      _transport = transport;
      requestRedraw();
   }

   void ImGuiGui::updateTrackInfo(bool hasTrackInfo, const clap_track_info &info) {
      if (hasTrackInfo)
         _trackInfo = info;
      else
         _trackInfo.reset();
      requestRedraw();
   }

   ////////////
   // Window //
   ////////////
   bool ImGuiGui::attachWindow(std::unique_ptr<PlatformWindow> window) {
      if (_window || !window)
         return false;

      _window = std::move(window);
      _window->setMinSize(kMinSize, kMinSize);
      _window->setSize(_width, _height);
      _window->setTitle(_title);
      _window->setVisible(_isVisible);
      requestRedraw();
      return true;
   }

   bool ImGuiGui::openWindow() {
#ifdef __linux__
      if (_window)
         return true;

      auto window = std::make_unique<WaylandWindow>(windowListener());
      if (!window->open())
         return false;

      auto *waylandWindow = window.get();
      if (!attachWindow(std::move(window)))
         return false;

      _waylandWindow = waylandWindow;
      return true;
#else
      return false;
#endif
   }

   bool ImGuiGui::setTransientWayland(const char *exportedHandle) {
#ifdef __linux__
      return _waylandWindow && _waylandWindow->setTransient(exportedHandle);
#else
      return false;
#endif
   }

   void ImGuiGui::setWaylandActivationToken(const char *token) {
#ifdef __linux__
      if (_waylandWindow)
         _waylandWindow->setActivationToken(token);
#endif
   }

   void ImGuiGui::setTitle(const std::string &title) {
      _title = title;
      if (_window)
         _window->setTitle(title);
      requestRedraw();
   }

   bool ImGuiGui::getSize(uint32_t *width, uint32_t *height) {
      *width = _width;
      *height = _height;
      return true;
   }

   bool ImGuiGui::roundSize(uint32_t *width, uint32_t *height) {
      *width = std::max(*width, kMinSize);
      *height = std::max(*height, kMinSize);
      return true;
   }

   bool ImGuiGui::setSize(uint32_t width, uint32_t height) {
      roundSize(&width, &height);
      _width = width;
      _height = height;
      if (_window)
         _window->setSize(width, height);
      requestRedraw();
      return true;
   }

   bool ImGuiGui::show() {
      _isVisible = true;
      if (_window)
         _window->setVisible(true);
      requestRedraw();
      return true;
   }

   bool ImGuiGui::hide() {
      _isVisible = false;
      if (_window)
         _window->setVisible(false);
      return true;
   }

   ///////////
   // Input //
   ///////////
   void ImGuiGui::onPointerMove(double x, double y) {
      ContextScope scope(_context);
      ImGui::GetIO().AddMousePosEvent(float(x), float(y));
      requestRedraw();
   }

   void ImGuiGui::onPointerLeave() {
      ContextScope scope(_context);
      ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
      requestRedraw();
   }

   void ImGuiGui::onPointerButton(int button, bool isPressed) {
      ContextScope scope(_context);
      ImGui::GetIO().AddMouseButtonEvent(button, isPressed);
      requestRedraw();
   }

   void ImGuiGui::onPointerScroll(double dx, double dy) {
      ContextScope scope(_context);
      ImGui::GetIO().AddMouseWheelEvent(float(dx), float(dy));
      requestRedraw();
   }

   void ImGuiGui::onScaleChanged() { requestRedraw(); }

   void ImGuiGui::onResized(uint32_t width, uint32_t height) {
      _width = width;
      _height = height;
      requestRedraw();
   }

   void ImGuiGui::onCloseRequested() { _isCloseRequested = true; }

   ///////////////
   // Rendering //
   ///////////////
   void ImGuiGui::requestRedraw() noexcept { _framesToRender = kFramesPerUpdate; }

   void ImGuiGui::onTimer() {
      if (_window)
         _window->dispatchEvents();

      // Gets the parameters values, transport, ... from the plugin
      _listener.onGuiPoll();

      if (_window && _isVisible && _framesToRender > 0)
         render();

      if (_isCloseRequested) {
         _isCloseRequested = false;
         hide();
         // Last thing we do: the host may destroy us
         _listener.onGuiWindowClosed(false);
      }
   }

   void ImGuiGui::render() {
      auto *fb = _window->beginFrame();
      if (!fb)
         return;

      ContextScope scope(_context);

      // The framebuffer size is rounded
      const float titleBarHeight = float(_window->titleBarHeight());
      auto &io = ImGui::GetIO();
      io.DisplaySize = ImVec2(float(_width), float(_height) + titleBarHeight);
      io.DisplayFramebufferScale =
         ImVec2(fb->width / io.DisplaySize.x, fb->height / io.DisplaySize.y);

      const auto now = std::chrono::steady_clock::now();
      io.DeltaTime = 1.f / 60;
      if (_lastFrameTime)
         io.DeltaTime =
            std::clamp(std::chrono::duration<float>(now - *_lastFrameTime).count(), 1e-4f, .1f);
      _lastFrameTime = now;

      ImGui::NewFrame();
      _hoveredParamInFrame = CLAP_INVALID_ID;

      if (titleBarHeight > 0)
         drawTitleBar(titleBarHeight);

      const uint32_t bg = _skin->backgroundColor;
      ImGui::PushStyleColor(ImGuiCol_WindowBg,
                            IM_COL32((bg >> 16) & 0xff, (bg >> 8) & 0xff, bg & 0xff, 0xff));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
      ImGui::SetNextWindowPos(ImVec2(0, titleBarHeight));
      ImGui::SetNextWindowSize(ImVec2(float(_width), float(_height)));
      ImGui::Begin("skin", nullptr, kFixedWindowFlags);
      _skin->draw(*this);
      ImGui::End();
      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor();

      ImGui::Render();
      SoftwareRenderer::render(*ImGui::GetDrawData(), *fb, bg);
      _window->endFrame();
      --_framesToRender;

      if (_hoveredParamInFrame != _hoveredParam) {
         _hoveredParam = _hoveredParamInFrame;
         _listener.onGuiParamIsHovered(_hoveredParam);
      }
   }

   // When the window system doesn't decorate the window: the title, and a close button.
   // Pressing anywhere else moves the window.
   void ImGuiGui::drawTitleBar(float height) {
      const float width = float(_width);
      const float closeWidth = std::min(height * 1.5f, width);

      ImGui::PushStyleColor(ImGuiCol_WindowBg, kTitleBarColor);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(ImVec2(width, height));
      ImGui::Begin("title-bar", nullptr, kFixedWindowFlags);

      auto *drawList = ImGui::GetWindowDrawList();
      const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

      ImGui::SetCursorPos(ImVec2(0, 0));
      ImGui::InvisibleButton("move", ImVec2(std::max(width - closeWidth, 1.f), height));
      if (ImGui::IsItemActivated())
         _window->beginMove();

      const ImVec2 textSize = ImGui::CalcTextSize(_title.c_str());
      const ImVec2 textPos(std::max((width - textSize.x) / 2, 8.f), (height - textSize.y) / 2);
      drawList->PushClipRect(ImVec2(0, 0), ImVec2(width - closeWidth, height), true);
      drawList->AddText(textPos, textColor, _title.c_str());
      drawList->PopClipRect();

      ImGui::SetCursorPos(ImVec2(width - closeWidth, 0));
      if (ImGui::InvisibleButton("close", ImVec2(closeWidth, height)))
         _isCloseRequested = true;

      const ImVec2 min = ImGui::GetItemRectMin();
      const ImVec2 max = ImGui::GetItemRectMax();
      if (ImGui::IsItemHovered())
         drawList->AddRectFilled(min, max, kCloseButtonHoveredColor);

      const ImVec2 center((min.x + max.x) / 2, (min.y + max.y) / 2);
      const float r = height * .18f;
      drawList->AddLine(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r, center.y + r), textColor, 1.5f);
      drawList->AddLine(ImVec2(center.x - r, center.y + r), ImVec2(center.x + r, center.y - r), textColor, 1.5f);

      ImGui::End();
      ImGui::PopStyleVar(3);
      ImGui::PopStyleColor();
   }

} // namespace clap
