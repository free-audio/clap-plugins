#include <algorithm>
#include <cmath>
#include <numbers>

#include <imgui.h>

#include "imgui-gui.hh"
#include "widgets.hh"

namespace clap {

   namespace {
      constexpr ImU32 kRingColor = IM_COL32(0x54, 0x4d, 0x63, 0xff);
      constexpr ImU32 kKnobColor = IM_COL32(0x0c, 0x00, 0x2b, 0xff);
      constexpr ImU32 kKnobColorAdjusting = IM_COL32(0x25, 0x00, 0x8b, 0xff);
      constexpr ImU32 kKnobColorHovered = IM_COL32(0x1b, 0x00, 0x64, 0xff);
      constexpr ImU32 kValueColor = IM_COL32(0xff, 0xff, 0xff, 0xff);
      constexpr ImU32 kModulationColor = IM_COL32(0x10, 0xb1, 0xca, 0xff);
      constexpr ImU32 kLabelColor = IM_COL32(0xff, 0xff, 0xff, 0xff);

      // Relative to the knob size
      constexpr float kModulationMargin = .05f;

      // The ring covers 300°, with the gap at the bottom
      constexpr float kPi = std::numbers::pi_v<float>;
      constexpr float kRingAngle = kPi * 5 / 3;
      constexpr float kRingStart = kPi / 2 + (2 * kPi - kRingAngle) / 2;

      constexpr float kLabelSpacing = 2;

      // Normalized value change per pixel
      constexpr double kDragSpeed = .01;
      constexpr double kFineDragSpeed = .001;

      float valueAngle(double normalizedValue) {
         return kRingStart + float(normalizedValue) * kRingAngle;
      }

      ImU32 toImColor(const clap_color &c) { return IM_COL32(c.red, c.green, c.blue, c.alpha); }

      // Annular sector between angles a0 and a1
      void drawArc(ImDrawList *dl, ImVec2 center, float innerRadius, float outerRadius, float a0, float a1, ImU32 color) {
         if (a0 == a1)
            return;

         dl->PathArcTo(center, (innerRadius + outerRadius) / 2, a0, a1);
         dl->PathStroke(color, outerRadius - innerRadius);
      }
   } // namespace

   float knobHeight(float size) { return size + kLabelSpacing + ImGui::GetTextLineHeight(); }

   void knob(ImGuiGui &gui, clap_id paramId, float size) {
      auto *p = gui.param(paramId);
      if (!p)
         return;

      ImGui::PushID(int(paramId));

      //////////////
      // Behavior //
      //////////////
      const ImVec2 pos = ImGui::GetCursorScreenPos();
      ImGui::InvisibleButton("knob", ImVec2(size, size));

      const bool isHovered = ImGui::IsItemHovered();
      if (ImGui::IsItemActivated())
         gui.beginAdjust(*p);

      if (ImGui::IsItemActive()) {
         const auto &io = ImGui::GetIO();
         if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            gui.setToDefault(*p);
         else if (io.MouseDelta.y != 0) {
            p->dragValue -= (io.KeyShift ? kFineDragSpeed : kDragSpeed) * io.MouseDelta.y;
            p->dragValue = std::clamp(p->dragValue, 0., 1.);
            gui.adjust(*p, p->denormalize(p->dragValue));
         }
      }

      if (ImGui::IsItemDeactivated())
         gui.endAdjust(*p);

      if (isHovered || ImGui::IsItemActive())
         gui.setHoveredParam(paramId);

      /////////////
      // Drawing //
      /////////////
      auto *dl = ImGui::GetWindowDrawList();
      const ImVec2 center = pos + ImVec2(size / 2, size / 2);
      const float outerRadius = size / 2;
      const float ringRadius = size / 2 - size * kModulationMargin;

      const ImU32 knobColor =
         p->isAdjusting ? kKnobColorAdjusting : (isHovered ? kKnobColorHovered : kKnobColor);
      dl->AddCircleFilled(center, size / 2 - 2 * kModulationMargin * size, knobColor);
      drawArc(dl, center, ringRadius, outerRadius, kRingStart, kRingStart + kRingAngle, kRingColor);

      const float angle = valueAngle(p->normalizedValue());
      drawArc(dl,
              center,
              ringRadius,
              outerRadius,
              angle,
              valueAngle(p->normalizedFinalValue()),
              kModulationColor);

      const ImVec2 dir(std::cos(angle), std::sin(angle));
      const float needleStart = size * kModulationMargin;
      const float needleEnd = size / 2 - size * (2 * kModulationMargin + .01f);
      dl->AddLine(center + dir * needleStart, center + dir * needleEnd, kValueColor, size / 20);
      dl->AddCircleFilled(center, size * (kModulationMargin + .03f), kValueColor);

      if (p->hasMappingIndication)
         dl->AddCircleFilled(center, size / 10, toImColor(p->mappingIndicationColor));

      if (p->automationIndicationState != CLAP_PARAM_INDICATION_AUTOMATION_NONE) {
         const float r = size / 20;
         dl->AddCircleFilled(
            pos + ImVec2(size - r, r), r, toImColor(p->automationIndicationColor));
      }

      ///////////
      // Label //
      ///////////
      const char *name = p->info.name;
      const float textWidth = ImGui::CalcTextSize(name).x;
      const ImVec2 labelPos(pos.x + std::max(0.f, (size - textWidth) / 2), pos.y + size + kLabelSpacing);
      const ImVec4 clip(pos.x, labelPos.y, pos.x + size, labelPos.y + ImGui::GetTextLineHeight());
      dl->AddText(nullptr, 0, labelPos, kLabelColor, name, nullptr, 0, &clip);
      ImGui::Dummy(ImVec2(size, knobHeight(size) - size));

      ImGui::PopID();
   }

} // namespace clap
