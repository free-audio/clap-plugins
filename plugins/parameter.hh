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
      explicit Parameter(const clap_param_info &info,
                         std::unique_ptr<ValueType> valueType = std::make_unique<SimpleValueType>());

      Parameter(const Parameter &) = delete;
      Parameter(Parameter &&) = delete;
      Parameter &operator=(const Parameter &) = delete;
      Parameter &operator=(Parameter &&) = delete;

      double value() const noexcept { return _value.value(); }
      double modulation() const noexcept { return _modulation.value(); }
      double modulatedValue() const noexcept { return _value.value() + _modulation.value(); }
      auto &valueType() const noexcept { return _valueType; }

      const clap_param_info &info() const noexcept { return _info; }

      void setDefaultValue() {
         setValueImmediately(_info.default_value);
         setModulationImmediately(0);
      }

      void setValueImmediately(double val) noexcept { _value.setImmediately(val); }
      void setModulationImmediately(double mod) { _modulation.setImmediately(mod); }

      void setValueSmoothed(double val, uint16_t steps) noexcept { _value.setSmoothed(val, steps); }

      void setModulationSmoothed(double mod, uint16_t steps) noexcept {
         _modulation.setSmoothed(mod, steps);
      }

      [[nodiscard]] bool valueNeedsProcessing() const noexcept { return _value.isSmoothing(); }
      [[nodiscard]] bool modulationNeedsProcessing() const noexcept {
         return _modulation.isSmoothing();
      }

      // Advances the value by 1 samples and return the new value + modulation
      double step() noexcept { return _value.step() + _modulation.step(); }

      // Advances the value by n samples and return the new value + modulation
      double step(uint32_t n) noexcept { return _value.step(n) + _modulation.step(n); }

      void renderValue(uint32_t frameCount) noexcept;
      void renderModulation(uint32_t frameCount) noexcept;
      void renderModulatedValue(uint32_t frameCount) noexcept;

      auto &valueBuffer() const noexcept { return _valueBuffer; }
      auto &modulationBuffer() const noexcept { return _modulationBuffer; }

      // valueType.toEngine(value + mod)
      auto &modulatedValueBuffer() const noexcept { return _modulatedValueBuffer; }

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

      AudioBuffer<double> _valueBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _modulationBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _modulatedValueBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _value;
      SmoothedValue _modulation;

   public:
      /* keep those hooks public */
      IntrusiveList::Hook _valueToProcessHook;
      IntrusiveList::Hook _modulationToProcessHook;
      IntrusiveList::Hook _modulatedValueToProcessHook;

   private:
#if 0
      struct VoiceData {
         bool hasValue;
         bool hasModulation;
         SmoothedValue value;
         SmoothedValue modulation;

         // When a parameter diverge, it should then be put into the linked list of parameters that
         // diverged in order to be reset when the voice ends
         IntrusiveList::Hook valueToResetHook;
         IntrusiveList::Hook modulationToResetHook;
      };

      Voices<VoiceData> _voices;
#endif
   };
} // namespace clap
