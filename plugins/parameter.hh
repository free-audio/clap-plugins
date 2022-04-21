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
      explicit Parameter(
         const clap_param_info &info,
         std::unique_ptr<ValueType> valueType = std::make_unique<SimpleValueType>());

      Parameter(const Parameter &) = delete;
      Parameter(Parameter &&) = delete;
      Parameter &operator=(const Parameter &) = delete;
      Parameter &operator=(Parameter &&) = delete;

      static const constexpr uint32_t POLY_AUTO_FLAGS = CLAP_PARAM_IS_AUTOMATABLE |
                                                        CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL |
                                                        CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE;
      static const constexpr uint32_t POLY_MOD_FLAGS = CLAP_PARAM_IS_MODULATABLE |
                                                       CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL |
                                                       CLAP_PARAM_IS_MODULATABLE_PER_NOTE;
      static const constexpr uint32_t POLY_FLAGS = POLY_AUTO_FLAGS | POLY_MOD_FLAGS;

      auto &valueType() const noexcept { return _valueType; }
      const clap_param_info &info() const noexcept { return _info; }

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
         if (_valueType->isStepped())
            _mainVoice._value.setImmediately(val);
         else
            _mainVoice._value.setSmoothed(val, steps);
      }

      void setModulationSmoothed(double mod, uint16_t steps) noexcept {
         if (_valueType->isStepped())
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

      auto &mainBuffer() const noexcept { return _mainVoice._modulatedValueBuffer; }
      auto &voiceBuffer(uint32_t voiceIndex) const noexcept {
         if (_voices[voiceIndex]._hasModulatedValue) [[unlikely]]
            return _voices[voiceIndex]._modulatedValueBuffer;
         return _mainVoice._modulatedValueBuffer;
      }
      auto &voiceBuffer(const VoiceModule *voice) const noexcept {
         if (voice) [[likely]]
            return voiceBuffer(voice->voiceIndex());
         return mainBuffer();
      }

      [[nodiscard]] bool hasGuiOverride() const noexcept { return _hasGuiOverride; }
      void setHasGuiOverride(bool isOverriden) noexcept { _hasGuiOverride = isOverriden; }

   private:
      clap_param_info _info;
      std::unique_ptr<ValueType> _valueType;

      bool _hasGuiOverride = false;

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

      Voice _mainVoice;
      std::array<Voice, MAX_VOICES> _voices;
   };
} // namespace clap
