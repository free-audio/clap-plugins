#include "parameter.hh"

namespace clap {
   Parameter::Parameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType)
      : _info(info), _valueType(std::move(valueType)) {
      _info.cookie = this;
      setDefaultValue();

      _main._param = this;
      _main._isMain = true;
      _main._voiceIndex = -1;
      _main._hasValue = true;
      _main._hasModulation = true;

      for (uint32_t i = 0; i < _voices.size(); ++i) {
         auto &voice = _voices[i];
         voice._param = this;
         voice._isMain = false;
         voice._voiceIndex = i;
         voice._hasValue = false;
         voice._hasModulation = false;
      }
   }

   void Parameter::VoiceData::renderValue(uint32_t frameCount) noexcept {
      if (_value.isSmoothing()) {
         _value.render(_valueBuffer.data(), frameCount);
         _valueBuffer.setConstant(false);
      } else {
         _valueBuffer.setConstantValue(_value.value());
         _valueToProcessHook.unlink();
      }
   }

   void Parameter::VoiceData::renderModulation(uint32_t frameCount) noexcept {
      if (_modulation.isSmoothing()) {
         _modulation.render(_modulationBuffer.data(), frameCount);
         _modulationBuffer.setConstant(false);
      } else {
         _modulationBuffer.data()[0] = _modulation.value();
         _modulationBuffer.setConstantValue(_modulation.value());
         _modulationToProcessHook.unlink();
      }
   }

   void Parameter::VoiceData::renderModulatedValue(uint32_t frameCount) noexcept {
      _modulatedValueBuffer.sum(_valueBuffer, _modulationBuffer, frameCount);
      _param->_valueType->toEngine(_modulatedValueBuffer, frameCount);

      if (!_value.isSmoothing() && !_modulation.isSmoothing() && _modulatedValueBuffer.isConstant())
         _modulatedValueToProcessHook.unlink();
   }
} // namespace clap
