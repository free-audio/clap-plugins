#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <variant>

#include <imgui.h>

#include "skins.hh"
#include "widgets.hh"

namespace clap {

   namespace {
      constexpr uint32_t kDarkBackground = 0x224477;
      constexpr uint32_t kLightBackground = 0xf8f8f8;
      constexpr ImU32 kDarkTextColor = IM_COL32(0x10, 0x10, 0x10, 0xff);

      // For the skins which show a block of text
      constexpr float kLargeFontSize = 24;
      constexpr float kTextMargin = 10;

      // See Module::parameter_capacity
      constexpr clap_id kModuleParamCapacity = 1024;

      clap_id moduleParamId(clap_id moduleId, clap_id index) {
         return moduleId * kModuleParamCapacity + index;
      }

      // Black text on the light background
      class LightStyle {
      public:
         LightStyle() {
            ImGui::PushStyleColor(ImGuiCol_Text, kDarkTextColor);
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, IM_COL32(0x80, 0x80, 0x80, 0xff));
         }
         ~LightStyle() { ImGui::PopStyleColor(2); }
      };

      void largeText(const std::string &text) {
         LightStyle style;
         ImGui::PushFont(nullptr, kLargeFontSize);
         ImGui::SetCursorPos(ImVec2(kTextMargin, kTextMargin));
         ImGui::TextUnformatted(text.c_str());
         ImGui::PopFont();
      }

      std::string format(const char *fmt, auto... args) {
         char buffer[256];
         snprintf(buffer, sizeof(buffer), fmt, args...);
         return buffer;
      }

      const char *toString(bool value) { return value ? "true" : "false"; }

      std::string toString(const InvocationArgumentType &value) {
         return std::visit(
            [](const auto &v) -> std::string {
               using T = std::decay_t<decltype(v)>;
               if constexpr (std::is_same_v<T, bool>)
                  return toString(v);
               else if constexpr (std::is_same_v<T, int64_t>)
                  return format("%" PRId64, v);
               else if constexpr (std::is_same_v<T, double>)
                  return format("%g", v);
               else
                  return v;
            },
            value);
      }

      // The default buttons are translucent, which lacks contrast on the light background
      bool button(const char *label, bool isEnabled) {
         ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
         ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0x22, 0x44, 0x77, 0xff));
         ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0x2d, 0x5a, 0x9e, 0xff));
         ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0x18, 0x30, 0x55, 0xff));
         ImGui::BeginDisabled(!isEnabled);
         const bool isClicked = ImGui::Button(label);
         ImGui::EndDisabled();
         ImGui::PopStyleColor(4);
         return isClicked;
      }

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

      ////////////////////////////////////
      // One big knob: gain, dc-offset //
      ////////////////////////////////////
      void singleKnobSize(const ImGuiGui &, uint32_t *width, uint32_t *height) {
         *width = 200;
         *height = 200;
      }

      void singleKnobDraw(ImGuiGui &gui) {
         constexpr float margin = 20;
         const float size = std::max(
            std::min(gui.width() - 2 * margin, gui.height() - 2 * margin - knobHeight(0)), 16.f);
         ImGui::SetCursorPos(
            ImVec2((gui.width() - size) / 2, (gui.height() - knobHeight(size)) / 2));
         knob(gui, 0, size);
      }

      ////////////////
      // Char Check //
      ////////////////
      void charCheckSize(const ImGuiGui &, uint32_t *width, uint32_t *height) {
         *width = 280;
         *height = 150;
      }

      void charCheckDraw(ImGuiGui &gui) { largeText("Characters check\nSee parameters name"); }

      ////////////////
      // Track Info //
      ////////////////
      void textBlockSize(const ImGuiGui &, uint32_t *width, uint32_t *height) {
         *width = 450;
         *height = 490;
      }

      void trackInfoDraw(ImGuiGui &gui) {
         const auto &info = gui.trackInfo();
         if (!info) {
            largeText("No info");
            return;
         }

         const auto &c = info->color;
         std::string text;
         text += format("Name: %s\n", info->name);
         text += format("Color: #%02x%02x%02x%02x\n", c.red, c.green, c.blue, c.alpha);
         text += format("Flags: %" PRIu64 "\n", info->flags);
         if (info->flags & CLAP_TRACK_INFO_HAS_AUDIO_CHANNEL) {
            text += format("Audio Port Type: %s\n",
                           info->audio_port_type ? info->audio_port_type : "(none)");
            text += format("Audio Channel Count: %d\n", info->audio_channel_count);
         } else
            text += "Audio Port Type: (none)\nAudio Channel Count: (none)\n";
         text += format("IsForReturn: %s\n",
                        toString(info->flags & CLAP_TRACK_INFO_IS_FOR_RETURN_TRACK));
         text += format("IsForBus: %s\n", toString(info->flags & CLAP_TRACK_INFO_IS_FOR_BUS));
         text +=
            format("IsForMaster: %s\n", toString(info->flags & CLAP_TRACK_INFO_IS_FOR_MASTER));
         largeText(text);
      }

      ////////////////////
      // Transport Info //
      ////////////////////
      double beats(clap_beattime t) { return double(t) / CLAP_BEATTIME_FACTOR; }
      double seconds(clap_sectime t) { return double(t) / CLAP_SECTIME_FACTOR; }

      void transportInfoDraw(ImGuiGui &gui) {
         const auto &t = gui.transport();
         if (!t) {
            largeText("Free Running");
            return;
         }

         const bool hasBeats = t->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
         const bool hasSeconds = t->flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE;

         std::string text;
         if (t->flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE)
            text += format("Time Signature: %d/%d\n", t->tsig_num, t->tsig_denom);
         else
            text += "Time Signature: (none)\n";

         if (t->flags & CLAP_TRANSPORT_HAS_TEMPO)
            text += format("Tempo: %.3f (bpm)\n", t->tempo);
         else
            text += "Tempo: (none)\n";

         if (hasBeats) {
            text += format("song position (beats): %.3f\n", beats(t->song_pos_beats));
            text += format("bar start: %.3f\n", beats(t->bar_start));
            text += format("bar number: %d\n", t->bar_number);
         } else
            text += "No timeline in beats\n";

         if (hasSeconds)
            text += format("song position (seconds): %.3fs\n", seconds(t->song_pos_seconds));
         else
            text += "No timeline in seconds\n";

         text += "\n";
         text += format("Is playing: %s\n", toString(t->flags & CLAP_TRANSPORT_IS_PLAYING));
         text += format("Is recording: %s\n", toString(t->flags & CLAP_TRANSPORT_IS_RECORDING));
         text += format("Is within preroll: %s\n",
                        toString(t->flags & CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL));

         if (t->flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE) {
            text += "\n";
            if (hasBeats)
               text += format("Loop (beats): %.3f .. %.3f\n",
                              beats(t->loop_start_beats),
                              beats(t->loop_end_beats));
            if (hasSeconds)
               text += format("Loop (seconds): %.3fs .. %.3fs\n",
                              seconds(t->loop_start_seconds),
                              seconds(t->loop_end_seconds));
         }

         largeText(text);
      }

      ///////////////
      // Undo Test //
      ///////////////
      void undoTestSize(const ImGuiGui &, uint32_t *width, uint32_t *height) {
         *width = 450;
         *height = 300;
      }

      void undoTestDraw(ImGuiGui &gui) {
         LightStyle style;
         ImGui::SetCursorPos(ImVec2(kTextMargin, kTextMargin));
         ImGui::BeginGroup();

         ImGui::PushFont(nullptr, kLargeFontSize);
         ImGui::TextUnformatted("Undo Test");
         ImGui::PopFont();

         const bool undo = button("Undo", gui.canUndo());
         ImGui::SameLine();
         ImGui::TextUnformatted(gui.undoName().c_str());

         const bool redo = button("Redo", gui.canRedo());
         ImGui::SameLine();
         ImGui::TextUnformatted(gui.redoName().c_str());

         const bool doSomething = button("Do something", true);

         const auto *counter = gui.property("counter");
         ImGui::Text("Counter: %s", counter ? toString(*counter).c_str() : "(none)");

         ImGui::EndGroup();

         // After the frame's widgets: the plugin may change the state right away
         if (undo)
            gui.undo();
         if (redo)
            gui.redo();
         if (doSomething)
            gui.invoke("incrementState");
      }

      ///////////
      // Synth //
      ///////////
      constexpr float kSynthMargin = 10;
      constexpr float kSynthKnobSize = 40;
      constexpr float kSynthBigKnobSize = 60;
      constexpr float kSynthCellWidth = kSynthKnobSize + 8;
      constexpr float kSynthGroupPadding = 8;
      constexpr float kSynthGroupTitleHeight = 18;
      constexpr ImU32 kSynthGroupColor = IM_COL32(0x80, 0x99, 0xc0, 0xff);

      // Module ids, see synth.cc
      constexpr clap_id kFilterAdsrModule = 2;
      constexpr clap_id kFilterModule = 3;
      constexpr clap_id kOsc1Module = 4;
      constexpr clap_id kOsc2Module = 5;
      constexpr clap_id kAmpAdsrModule = 6;
      constexpr clap_id kVoiceModule = 7;

      float synthCellHeight() { return knobHeight(kSynthKnobSize) + 4; }

      // A titled frame, whose content is the given size
      ImVec2 groupSize(ImVec2 content) {
         return ImVec2(content.x + 2 * kSynthGroupPadding,
                       content.y + kSynthGroupTitleHeight + kSynthGroupPadding);
      }

      // Returns the position of the content
      ImVec2 drawGroup(const char *title, ImVec2 pos, ImVec2 content) {
         auto *dl = ImGui::GetWindowDrawList();
         const ImVec2 origin = ImGui::GetWindowPos() + pos;
         const ImVec2 size = groupSize(content);
         dl->AddRect(origin + ImVec2(0, kSynthGroupTitleHeight / 2), origin + size, kSynthGroupColor, 4);

         const ImVec2 textPos = origin + ImVec2(kSynthGroupPadding, 0);
         const ImVec2 textSize = ImGui::CalcTextSize(title);
         dl->AddRectFilled(textPos - ImVec2(3, 0),
                           textPos + ImVec2(textSize.x + 3, textSize.y),
                           IM_COL32((kDarkBackground >> 16) & 0xff,
                                    (kDarkBackground >> 8) & 0xff,
                                    kDarkBackground & 0xff,
                                    0xff));
         dl->AddText(textPos, kSynthGroupColor, title);

         return pos + ImVec2(kSynthGroupPadding, kSynthGroupTitleHeight);
      }

      // Knobs in a row, the parameters being the module's ones
      ImVec2 knobRowSize(size_t count) {
         return ImVec2(count * kSynthCellWidth - (kSynthCellWidth - kSynthKnobSize),
                       knobHeight(kSynthKnobSize));
      }

      void knobRow(ImGuiGui &gui, ImVec2 pos, clap_id moduleId, std::initializer_list<clap_id> indexes) {
         float x = pos.x;
         for (auto index : indexes) {
            ImGui::SetCursorPos(ImVec2(x, pos.y));
            knob(gui, moduleParamId(moduleId, index), kSynthKnobSize);
            x += kSynthCellWidth;
         }
      }

      struct SynthLayout {
         ImVec2 osc1, osc2, mixer, filter, filterAdsr, ampAdsr, volume;
         ImVec2 oscContent, mixerContent, filterContent, adsrContent;
         ImVec2 size;

         SynthLayout() {
            oscContent = knobRowSize(2);
            mixerContent = ImVec2(kSynthKnobSize, 2 * synthCellHeight());
            filterContent = ImVec2(knobRowSize(2).x,
                                   knobHeight(kSynthBigKnobSize) + 4 + 2 * synthCellHeight());
            adsrContent = knobRowSize(5);

            const ImVec2 oscGroup = groupSize(oscContent);
            const ImVec2 mixerGroup = groupSize(mixerContent);
            const ImVec2 filterGroup = groupSize(filterContent);
            const ImVec2 adsrGroup = groupSize(adsrContent);

            float x = kSynthMargin;
            const float y = kSynthMargin;
            osc1 = ImVec2(x, y);
            osc2 = ImVec2(x, y + oscGroup.y + kSynthMargin);
            x += oscGroup.x + kSynthMargin;
            mixer = ImVec2(x, y);
            x += mixerGroup.x + kSynthMargin;
            filter = ImVec2(x, y);
            x += filterGroup.x + kSynthMargin;
            filterAdsr = ImVec2(x, y);
            ampAdsr = ImVec2(x, y + adsrGroup.y + kSynthMargin);
            x += adsrGroup.x + kSynthMargin;
            volume = ImVec2(x, y + kSynthGroupTitleHeight);
            x += kSynthKnobSize + kSynthMargin;

            const float height = std::max({2 * oscGroup.y + kSynthMargin,
                                           mixerGroup.y,
                                           filterGroup.y,
                                           2 * adsrGroup.y + kSynthMargin});
            size = ImVec2(x, height + 2 * kSynthMargin);
         }
      };

      void synthSize(const ImGuiGui &, uint32_t *width, uint32_t *height) {
         const SynthLayout layout;
         *width = uint32_t(std::ceil(layout.size.x));
         *height = uint32_t(std::ceil(layout.size.y));
      }

      void synthDraw(ImGuiGui &gui) {
         const SynthLayout l;

         knobRow(gui, drawGroup("Osc1", l.osc1, l.oscContent), kOsc1Module, {0, 1});
         knobRow(gui, drawGroup("Osc2", l.osc2, l.oscContent), kOsc2Module, {0, 1});

         const ImVec2 mixer = drawGroup("Mixer", l.mixer, l.mixerContent);
         knobRow(gui, mixer, kVoiceModule, {0});
         knobRow(gui, mixer + ImVec2(0, synthCellHeight()), kVoiceModule, {1});

         const ImVec2 filter = drawGroup("Filter", l.filter, l.filterContent);
         ImGui::SetCursorPos(
            filter + ImVec2((l.filterContent.x - kSynthBigKnobSize) / 2, 0));
         knob(gui, moduleParamId(kFilterModule, 0), kSynthBigKnobSize);
         const ImVec2 grid = filter + ImVec2(0, knobHeight(kSynthBigKnobSize) + 4);
         knobRow(gui, grid, kFilterModule, {5, 1}); // fm, reso
         knobRow(gui, grid + ImVec2(0, synthCellHeight()), kFilterModule, {3, 4}); // kt, env

         knobRow(gui, drawGroup("FEG", l.filterAdsr, l.adsrContent), kFilterAdsrModule, {0, 1, 2, 3, 4});
         knobRow(gui, drawGroup("AEG", l.ampAdsr, l.adsrContent), kAmpAdsrModule, {0, 1, 2, 3, 4});

         knobRow(gui, l.volume, kVoiceModule, {2});
      }

      const Skin skins[] = {
         {"", kDarkBackground, false, genericSize, genericDraw},
         {"gain", kDarkBackground, false, singleKnobSize, singleKnobDraw},
         {"dc-offset", kDarkBackground, false, singleKnobSize, singleKnobDraw},
         {"char-check", kLightBackground, false, charCheckSize, charCheckDraw},
         {"track-info", kLightBackground, false, textBlockSize, trackInfoDraw},
         {"transport-info", kLightBackground, true, textBlockSize, transportInfoDraw},
         {"undo-test", kLightBackground, false, undoTestSize, undoTestDraw},
         {"synth", kDarkBackground, false, synthSize, synthDraw},
      };
   } // namespace

   const Skin &findSkin(const std::string &name) {
      for (auto &skin : skins)
         if (name == skin.name)
            return skin;
      return skins[0];
   }

} // namespace clap
