#include <algorithm>
#include <cmath>
#include <cstring>

#include <imgui.h>

#include "skins.hh"
#include "widgets.hh"

namespace clap {

   namespace {

      /////////////
      // Generic //
      /////////////
      constexpr float kGenericMargin = 10;
      constexpr float kGenericCellWidth = 80;
      constexpr float kGenericCellHeight = 90;
      constexpr float kGenericKnobSize = 56;
      constexpr size_t kGenericMaxColumns = 6;

      void genericSize(const ImGuiGui &gui, uint32_t *width, uint32_t *height) {
         const size_t count = std::max<size_t>(gui.paramIds().size(), 1);
         const size_t columns = std::min(count, kGenericMaxColumns);
         const size_t rows = (count + columns - 1) / columns;
         *width = uint32_t(std::max(2 * kGenericMargin + columns * kGenericCellWidth, 200.f));
         *height = uint32_t(std::max(2 * kGenericMargin + rows * kGenericCellHeight, 100.f));
      }

      void genericDraw(ImGuiGui &gui) {
         const auto &ids = gui.paramIds();
         if (ids.empty()) {
            ImGui::SetCursorPos(ImVec2(kGenericMargin, kGenericMargin));
            ImGui::TextUnformatted("No parameters");
            return;
         }

         const size_t columns =
            std::max(1, int((gui.width() - 2 * kGenericMargin) / kGenericCellWidth));
         for (size_t i = 0; i < ids.size(); ++i) {
            const float x = kGenericMargin + (i % columns) * kGenericCellWidth +
                            (kGenericCellWidth - kGenericKnobSize) / 2;
            const float y = kGenericMargin + (i / columns) * kGenericCellHeight;
            ImGui::SetCursorPos(ImVec2(x, y));
            knob(gui, ids[i], kGenericKnobSize);
         }
      }

      //////////
      // Gain //
      //////////
      constexpr clap_id kGainParamId = 0;

      void gainSize(const ImGuiGui &, uint32_t *width, uint32_t *height) {
         *width = 200;
         *height = 200;
      }

      void gainDraw(ImGuiGui &gui) {
         constexpr float margin = 20;
         const float size = std::max(
            std::min(gui.width() - 2 * margin, gui.height() - 2 * margin - knobHeight(0)), 16.f);
         ImGui::SetCursorPos(
            ImVec2((gui.width() - size) / 2, (gui.height() - knobHeight(size)) / 2));
         knob(gui, kGainParamId, size);
      }

      const Skin skins[] = {
         {"", 0x224477, genericSize, genericDraw},
         {"gain", 0x224477, gainSize, gainDraw},
      };
   } // namespace

   const Skin &findSkin(const std::string &name) {
      for (auto &skin : skins)
         if (name == skin.name)
            return skin;
      return skins[0];
   }

} // namespace clap
