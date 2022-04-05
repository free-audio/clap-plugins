#include "parameter.hh"

namespace clap {
   Parameter::Parameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType)
      : _info(info), _valueType(std::move(valueType)) {
      _info.cookie = this;
      setDefaultValue();
   }

   void Parameter::renderValue(uint32_t frameCount) noexcept {
      if (_value.isSmoothing()) {
         _value.render(_valueBuffer.data(), frameCount);
         _valueBuffer.setConstant(false);
      } else {
         _valueBuffer.data()[0] = _value.value();
         _valueBuffer.setConstant(true);
         _valueToProcessHook.unlink();
      }
   }

   void Parameter::renderModulation(uint32_t frameCount) noexcept {
      if (_modulation.isSmoothing()) {
         _modulation.render(_modulationBuffer.data(), frameCount);
         _modulationBuffer.setConstant(false);
      } else {
         _modulationBuffer.data()[0] = _modulation.value();
         _modulationBuffer.setConstant(true);
         _modulationToProcessHook.unlink();
      }
   }

   void Parameter::renderModulatedValue(uint32_t frameCount) noexcept {
      _modulatedValueBuffer.sum(_valueBuffer, _modulationBuffer, frameCount);
      _valueType->toEngine(_modulatedValueBuffer, frameCount);

      if (!_value.isSmoothing() && !_modulation.isSmoothing())
         _modulatedValueToProcessHook.unlink();
   }
} // namespace clap
