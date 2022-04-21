#include "parameter.hh"

namespace clap {
   Parameter::Parameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType)
      : _info(info), _valueType(std::move(valueType)) {
      _info.cookie = this;
      reset();

      _mainVoice._param = this;
      _mainVoice._isMain = true;
      _mainVoice._voiceIndex = -1;
      _mainVoice._hasValue = true;
      _mainVoice._hasModulation = true;
      _mainVoice._hasModulatedValue = true;

      for (uint32_t i = 0; i < _voices.size(); ++i) {
         auto &voice = _voices[i];
         voice._param = this;
         voice._isMain = false;
         voice._voiceIndex = i;
         voice._hasValue = false;
         voice._hasModulation = false;
         voice._hasModulatedValue = false;
      }
   }

   void Parameter::Voice::renderValue(uint32_t frameCount) noexcept {
      assert(_hasValue);

      if (_value.isSmoothing()) {
         _value.render(_valueBuffer.data(), frameCount);
         _valueBuffer.setConstant(false);
      } else {
         _valueBuffer.setConstantValue(_value.value());
         _valueToProcessHook.unlink();
      }
   }

   void Parameter::Voice::renderModulation(uint32_t frameCount) noexcept {
      assert(_hasModulation);

      if (_modulation.isSmoothing()) {
         _modulation.render(_modulationBuffer.data(), frameCount);
         _modulationBuffer.setConstant(false);
      } else {
         _modulationBuffer.data()[0] = _modulation.value();
         _modulationBuffer.setConstantValue(_modulation.value());
         _modulationToProcessHook.unlink();
      }
   }

   void Parameter::Voice::renderModulatedValue(uint32_t frameCount) noexcept {
      AudioBuffer<double> &valueBuffer = _hasValue ? _valueBuffer : _param->_mainVoice._valueBuffer;
      AudioBuffer<double> &modulationBuffer = _hasModulation ? _modulationBuffer : _param->_mainVoice._modulationBuffer;

      _modulatedValueBuffer.sum(valueBuffer, modulationBuffer, frameCount);
      _param->_valueType->toEngine(_modulatedValueBuffer, frameCount);

      if (!_value.isSmoothing() && !_modulation.isSmoothing() && _modulatedValueBuffer.isConstant())
         _modulatedValueToProcessHook.unlink();
   }
} // namespace clap
