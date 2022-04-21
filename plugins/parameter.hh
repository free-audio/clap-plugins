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

      double value() const noexcept { return _main._value.value(); }
      double modulation() const noexcept { return _main._modulation.value(); }
      double modulatedValue() const noexcept { return _main._value.value() +_main. _modulation.value(); }
      auto &valueType() const noexcept { return _valueType; }

      const clap_param_info &info() const noexcept { return _info; }

      void setDefaultValue() {
         setValueImmediately(_info.default_value);
         setModulationImmediately(0);
      }

      void setValueImmediately(double val) noexcept { _main._value.setImmediately(val); }
      void setModulationImmediately(double mod) { _main._modulation.setImmediately(mod); }

      void setValueSmoothed(double val, uint16_t steps) noexcept {
         if (_valueType->isStepped())
            _main._value.setImmediately(val);
         else
            _main._value.setSmoothed(val, steps);
      }

      void setModulationSmoothed(double mod, uint16_t steps) noexcept {
         if (_valueType->isStepped())
            _main._modulation.setImmediately(mod);
         else
            _main._modulation.setSmoothed(mod, steps);
      }

      [[nodiscard]] bool valueNeedsProcessing() const noexcept { return _main._value.isSmoothing(); }
      [[nodiscard]] bool modulationNeedsProcessing() const noexcept {
         return _main._modulation.isSmoothing();
      }

      // Advances the value by 1 samples and return the new value + modulation
      double step() noexcept { return _main._value.step() + _main._modulation.step(); }

      // Advances the value by n samples and return the new value + modulation
      double step(uint32_t n) noexcept { return _main._value.step(n) + _main._modulation.step(n); }

      auto &valueBuffer() const noexcept { return _main._valueBuffer; }
      auto &modulationBuffer() const noexcept { return _main._modulationBuffer; }

      // valueType.toEngine(value + mod)
      auto &modulatedValueBuffer() const noexcept { return _main._modulatedValueBuffer; }

#if 0
   private:
      auto &getVoiceData(uint32_t voiceIndex) noexcept {
         assert(voiceIndex < _voices.size());
         return _voices[voiceIndex];
      }

   public:
      SmoothedValue &getVoiceValue(uint32_t voiceIndex) noexcept {
         auto &v = getVoiceData(voiceIndex);
         return v.hasValue ? v.value : _value;
      }

      SmoothedValue &getVoiceModulation(uint32_t voiceIndex) noexcept {
         auto &v = getVoiceData(voiceIndex);
         return v.hasModulation ? v.modulation : _modulation;
      }
#endif

      [[nodiscard]] bool hasGuiOverride() const noexcept { return _hasGuiOverride; }
      void setHasGuiOverride(bool isOverriden) noexcept { _hasGuiOverride = isOverriden; }

   private:
      clap_param_info _info;
      std::unique_ptr<ValueType> _valueType;

      bool _hasGuiOverride = false;

   public:
      struct VoiceData {
         void renderValue(uint32_t frameCount) noexcept;
         void renderModulation(uint32_t frameCount) noexcept;
         void renderModulatedValue(uint32_t frameCount) noexcept;

         Parameter *_param = nullptr;

         bool _isMain = false;
         uint32_t _voiceIndex = -1;

         bool _hasValue = false;
         bool _hasModulation = false;

         SmoothedValue _value;
         SmoothedValue _modulation;

         AudioBuffer<double> _valueBuffer;
         AudioBuffer<double> _modulationBuffer;
         AudioBuffer<double> _modulatedValueBuffer;

         // When a parameter diverge, it should then be put into the linked list of parameters that
         // diverged in order to be reset when the voice ends
         IntrusiveList::Hook _valueToResetHook;
         IntrusiveList::Hook _valueToProcessHook;
         IntrusiveList::Hook _modulationToResetHook;
         IntrusiveList::Hook _modulationToProcessHook;
         IntrusiveList::Hook _modulatedValueToResetHook;
         IntrusiveList::Hook _modulatedValueToProcessHook;
      };

      VoiceData _main;
      std::array<VoiceData, MAX_VOICES> _voices;
   };
} // namespace clap
