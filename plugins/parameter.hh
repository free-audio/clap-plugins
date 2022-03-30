#pragma once

#include <array>
#include <bitset>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <clap/clap.h>

#include "smoothed-value.hh"
#include "voice.hh"

namespace clap {
   class Voice;

   class Parameter {
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
      SmoothedValue &getVoiceValue(uint32_t voiceIndex) {
         assert(voiceIndex < _voiceValues.size());
         return _hasVoiceValue.test(voiceIndex) ? _voiceValues[voiceIndex] : _value;
      }

      SmoothedValue &getVoiceModulation(uint32_t voiceIndex) {
         assert(voiceIndex < _voiceModulations.size());
         return  _hasVoiceModulation.test(voiceIndex) ? _voiceModulations[voiceIndex] : _modulation;
      }

      clap_param_info _info;

      SmoothedValue _value;
      SmoothedValue _modulation;

      std::bitset<Voice::max_voices> _hasVoiceValue;
      std::bitset<Voice::max_voices> _hasVoiceModulation;
      std::array<SmoothedValue, Voice::max_voices> _voiceValues;
      std::array<SmoothedValue, Voice::max_voices> _voiceModulations;

      // When a parameter diverge, it should then be put into the linked list of parameters that
      // diverged in order to be reset when the voice ends
      std::array<Parameter *, Voice::max_voices> _voicesValueToResetHooks;
      std::array<Parameter *, Voice::max_voices> _voicesModulationToResetHooks;
   };
} // namespace clap
