#pragma once

#include <array>
#include <bitset>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <clap/clap.h>

#include "audio-buffer.hh"
#include "constants.hh"
#include "intrusive-list.hh"
#include "modules/voice-module.hh"
#include "smoothed-value.hh"
#include "value-types/simple-value-type.hh"

namespace clap {
   class CorePlugin;

   class Parameter final {
      friend class CorePlugin;

   public:
      explicit Parameter(const clap_param_info &info,
                         std::unique_ptr<ValueType> valueType,
                         uint32_t paramIndex);

      Parameter(const Parameter &) = delete;
      Parameter(Parameter &&) = delete;
      Parameter &operator=(const Parameter &) = delete;
      Parameter &operator=(Parameter &&) = delete;

      static const constexpr uint32_t POLY_AUTO_FLAGS =
         CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_AUTOMATABLE_PER_PORT |
         CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL | CLAP_PARAM_IS_AUTOMATABLE_PER_KEY |
         CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID;
      static const constexpr uint32_t POLY_MOD_FLAGS =
         CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_PORT |
         CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL | CLAP_PARAM_IS_MODULATABLE_PER_KEY |
         CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;
      static const constexpr uint32_t POLY_FLAGS = POLY_AUTO_FLAGS | POLY_MOD_FLAGS;

      auto &valueType() const noexcept { return _valueType; }
      const clap_param_info &info() const noexcept { return _info; }

      bool isSmoothingEnabled() const noexcept { return !_disableSmoothing; }
      void enableSmoothing(bool isEnabled) noexcept { _disableSmoothing = !isEnabled; }

      /* Mostly useful for the GUI */
      double value() const noexcept { return _mainVoice._value.value(); }
      double modulation() const noexcept { return _mainVoice._modulation.value(); }
      double modulatedValue() const noexcept {
         return _mainVoice._value.value() + _mainVoice._modulation.value();
      }

      void reset() {
         setValueImmediately(_info.default_value);
         setModulationImmediately(0);
      }

      void setValueImmediately(double val) noexcept { _mainVoice._value.setImmediately(val); }
      void setModulationImmediately(double mod) { _mainVoice._modulation.setImmediately(mod); }

      void setValueSmoothed(double val, uint16_t steps) noexcept {
         if (_valueType->isStepped() || _disableSmoothing)
            _mainVoice._value.setImmediately(val);
         else
            _mainVoice._value.setSmoothed(val, steps);
      }

      void setModulationSmoothed(double mod, uint16_t steps) noexcept {
         if (_valueType->isStepped() || _disableSmoothing)
            _mainVoice._modulation.setImmediately(mod);
         else
            _mainVoice._modulation.setSmoothed(mod, steps);
      }

      [[nodiscard]] bool valueNeedsProcessing() const noexcept {
         return _mainVoice._value.isSmoothing();
      }
      [[nodiscard]] bool modulationNeedsProcessing() const noexcept {
         return _mainVoice._modulation.isSmoothing();
      }

      const AudioBuffer<double> &mainBuffer() const noexcept {
         return _mainVoice._modulatedValueBuffer;
      }
      const AudioBuffer<double> &voiceBuffer(uint32_t voiceIndex) const noexcept;
      const AudioBuffer<double> &voiceBuffer(const VoiceModule *voice) const noexcept;

      [[nodiscard]] bool hasGuiOverride() const noexcept { return _hasGuiOverride; }
      void setHasGuiOverride(bool isOverriden) noexcept { _hasGuiOverride = isOverriden; }

      bool hasMappingIndication() const noexcept { return _hasMappingIndication; }
      const clap_color &mappingIndicationColor() const {
         assert(hasMappingIndication());
         return _mappingIndicationColor;
      }
      const std::string &mappingIndicationLabel() const noexcept { return _mappingIndicationLabel; }
      const std::string &mappingIndicationDescription() const noexcept {
         return _mappingIndicationDescription;
      }
      void setMappingIndication(const clap_color &color, const char *label, const char *desc);
      void clearMappingIndication();

      uint32_t automationIndicationState() const noexcept { return _automationState; }
      const clap_color &automationIndicationColor() const noexcept { return _automationColor; }
      void setAutomationIndication(uint32_t state, const clap_color &color) {
         _automationState = state;
         _automationColor = color;
      }

      int32_t getParamIndex() const noexcept { return _index; }

   private:
      // cached param index in the plugin
      const int32_t _index = -1;

      clap_param_info _info;
      std::unique_ptr<ValueType> _valueType;

      bool _hasGuiOverride = false;
      bool _hasMappingIndication = false;
      clap_color _mappingIndicationColor{0, 0, 0, 0};
      std::string _mappingIndicationLabel;
      std::string _mappingIndicationDescription;

      uint32_t _automationState = CLAP_PARAM_INDICATION_AUTOMATION_NONE;
      clap_color _automationColor{0, 0, 0, 0};

   public:
      struct Voice {
         ~Voice() {
            assert(!_valueToProcessHook.isHooked());
            assert(!_modulationToProcessHook.isHooked());
            assert(!_modulatedValueToProcessHook.isHooked());
         }

         void reset() {
            if (!_isMain) {
               _hasValue = false;
               _hasModulation = false;
               _hasModulatedValue = false;
            }
         }

         void renderValue(uint32_t frameCount) noexcept;
         void renderModulation(uint32_t frameCount) noexcept;
         void renderModulatedValue(uint32_t frameCount) noexcept;

         Parameter *_param = nullptr;

         bool _isMain = false;
         uint32_t _voiceIndex = -1;

         bool _hasValue = false;
         bool _hasModulation = false;
         bool _hasModulatedValue = false;

         SmoothedValue _value;
         SmoothedValue _modulation;

         AudioBuffer<double> _valueBuffer;
         AudioBuffer<double> _modulationBuffer;
         AudioBuffer<double> _modulatedValueBuffer;

         IntrusiveList::Hook _resetHook;
         IntrusiveList::Hook _valueToProcessHook;
         IntrusiveList::Hook _modulationToProcessHook;
         IntrusiveList::Hook _modulatedValueToProcessHook;
      };

      bool _disableSmoothing = false;
      Voice _mainVoice;
      std::array<Voice, MAX_VOICES> _voices;
   };
} // namespace clap
