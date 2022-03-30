#pragma once

#include <array>
#include <bitset>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <clap/clap.h>

#include "audio-buffer.hh"
#include "intrusive-list.hh"
#include "smoothed-value.hh"
#include "voice.hh"

namespace clap {
   class Voice;

   class Parameter final {
   public:
      explicit Parameter(const clap_param_info &info) : _info(info) { _info.cookie = this; }

      Parameter(const Parameter &) = delete;
      Parameter(Parameter &&) = delete;
      Parameter &operator=(const Parameter &) = delete;
      Parameter &operator=(Parameter &&) = delete;

      double value() const noexcept { return _value.value(); }
      double modulation() const noexcept { return _modulation.value(); }
      double modulatedValue() const noexcept { return _value.value() + _modulation.value(); }

      const clap_param_info &info() const noexcept { return _info; }

      void setDefaultValue() {
         _value.setImmediately(_info.default_value);
         _modulation.setImmediately(0);
      }

      void setValueImmediately(double val) noexcept { _value.setImmediately(val); }
      void setModulationImmediately(double mod) { _modulation.setImmediately(mod); }

      void setValueSmoothed(double val, uint16_t steps) noexcept { _value.setSmoothed(val, steps); }

      void setModulationSmoothed(double mod, uint16_t steps) noexcept {
         _modulation.setSmoothed(mod, steps);
      }

      // Advances the value by 1 samples and return the new value + modulation
      double step() noexcept { return _value.step() + _modulation.step(); }

      // Advances the value by n samples and return the new value + modulation
      double step(uint32_t n) { return _value.step(n) + _modulation.step(n); }

   private:
      auto &getVoiceData(uint32_t voiceIndex) noexcept {
         assert(voiceIndex < _voices.size());
         return _voices[voiceIndex];
      }

   public:
      SmoothedValue &getVoiceValue(uint32_t voiceIndex) {
         auto &v = getVoiceData(voiceIndex);
         return v.hasValue ? v.value : _value;
      }

      SmoothedValue &getVoiceModulation(uint32_t voiceIndex) {
         auto &v = getVoiceData(voiceIndex);
         return v.hasModulation ? v.modulation : _modulation;
      }

      [[nodiscard]] bool hasGuiOverride() const noexcept { return _hasGuiOverride; }
      void setHasGuiOverride(bool isOverriden) noexcept { _hasGuiOverride = isOverriden; }

   private:
      clap_param_info _info;

      bool _hasGuiOverride = false;

      std::unique_ptr<AudioBuffer<double>> _valueBuffer;
      SmoothedValue _value;
      SmoothedValue _modulation;

      IntrusiveListHook<Parameter> _valueToProcessHook;
      IntrusiveListHook<Parameter> _modulationToProcessHook;

      struct VoiceData {
         Parameter &param;
         bool hasValue;
         bool hasModulation;
         SmoothedValue value;
         SmoothedValue modulation;

         // When a parameter diverge, it should then be put into the linked list of parameters that
         // diverged in order to be reset when the voice ends
         IntrusiveListHook<VoiceData> valueToResetHook;
         IntrusiveListHook<VoiceData> modulationToResetHook;
      };

      Voices<VoiceData> _voices;
   };
} // namespace clap
