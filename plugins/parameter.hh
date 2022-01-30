#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <clap/clap.h>

#include "parameter-interpolator.hh"

namespace clap {
   class Parameter {
   public:
      explicit Parameter(const clap_param_info &info) : _info(info) { _info.cookie = this; }

      Parameter(const Parameter &) = delete;
      Parameter(Parameter &&) = delete;
      Parameter &operator=(const Parameter &) = delete;
      Parameter &operator=(Parameter &&) = delete;

      const double value() const noexcept { return _value; }
      const double modulation() const noexcept { return _modulation; }
      const double modulatedValue() const noexcept { return _value + _modulation; }
      const clap_param_info &info() const noexcept { return _info; }

      void setDefaultValue() {
         _value = _info.default_value;
         _modulation = 0;
      }

      void setValueImmediately(double val) {
         _value = val;
         _valueRamp = 0;
         _valueSteps = 0;
      }
      void setModulationImmediately(double mod) {
         _modulation = mod;
         _modulationRamp = 0;
         _modulationSteps = 0;
      }

      void setValueSmoothed(double val, uint16_t steps) {
         assert(steps > 0);
         _valueRamp = (val - _value) / steps;
         _valueSteps = steps;
      }

      void setModulationSmoothed(double mod, uint16_t steps) {
         assert(steps > 0);
         _modulationRamp = (mod - _modulation) / steps;
         _modulationSteps = steps;
      }

      // Advances the value by 1 samples and return the new value + modulation
      double step() {
         if (_valueSteps > 0) [[unlikely]] {
            _value += _valueRamp;
            --_valueSteps;
         }

         if (_modulationSteps > 0) [[unlikely]] {
            _modulation += _modulationRamp;
            --_modulationSteps;
         }

         return _value + _modulation;
      }

      // Advances the value by n samples and return the new value + modulation
      double step(uint32_t n) {
         if (_valueSteps > 0) [[unlikely]] {
            auto k = std::min<uint32_t>(_valueSteps, n);
            _value += k * _valueRamp;
            _valueSteps -= k;
         }

         if (_modulationSteps > 0) [[unlikely]] {
            auto k = std::min<uint32_t>(_valueSteps, n);
            _modulation += k * _modulationRamp;
            _modulationSteps -= k;
         }

         return _value + _modulation;
      }

   private:
      clap_param_info _info;

      double _value = 0;
      double _modulation = 0;

      double _valueRamp = 0;
      double _modulationRamp = 0;

      uint16_t _valueSteps = 0;
      uint16_t _modulationSteps = 0;
   };
} // namespace clap