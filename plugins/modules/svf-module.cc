#include <algorithm>
#include <cmath>
#include <vector>

#include "../tuning-utilities.hh"
#include "../value-types/enumerated-value-type.hh"
#include "../value-types/frequency-value-type.hh"
#include "svf-module.hh"
#include "voice-module.hh"

namespace clap {

   SvfModule::SvfModule(CorePlugin &plugin, std::string name, uint32_t moduleId)
      : Module(plugin, name, moduleId) {
      char moduleName[CLAP_MODULE_SIZE];
      snprintf(moduleName, sizeof(moduleName), "/%s", _name.c_str());

      _freqParam = addParameter(0,
                                "freq",
                                CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE,
                                std::make_unique<FrequencyValueType>(20, 20000, 400));
      _resoParam = addParameter(1,
                                "reso",
                                CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE,
                                std::make_unique<SimpleValueType>(0, 0.98, 0.3));
      _modeParam = addParameter(
         2,
         "mode",
         CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_STEPPED,
         std::make_unique<EnumeratedValueType>(std::vector<std::string>{"LP", "BP", "HP"}, 0));
      _keytrackParam = addParameter(3,
                                    "kt",
                                    CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE,
                                    std::make_unique<SimpleValueType>(0, 120, 0));
      _envParam = addParameter(4,
                               "env",
                               CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE,
                               std::make_unique<SimpleValueType>(-120, 120, 0));

      _fmParam = addParameter(5,
                              "mod",
                              CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE,
                              std::make_unique<SimpleValueType>(-120, 120, 0));

      _output.setConstant(false);
   }

   SvfModule::SvfModule(const SvfModule &m)
      : Module(m), _freqParam(m._freqParam), _resoParam(m._resoParam), _modeParam(m._modeParam),
        _keytrackParam(m._keytrackParam), _envParam(m._envParam), _fmParam(m._fmParam) {}

   SvfModule::~SvfModule() = default;

   std::unique_ptr<Module> SvfModule::cloneVoice() const {
      return std::make_unique<SvfModule>(*this);
   }

   bool SvfModule::doActivate(double sampleRate, uint32_t maxFrameCount) {
      _sampleRate = sampleRate;
      reset();
      return true;
   }

   void SvfModule::reset() noexcept {
      _ic1eq = 0;
      _ic2eq = 0;
      _lastFreq = 0;
      _lastReso = 0;
   }

   void SvfModule::setFilter(double freq, double reso) {
      freq = std::clamp<double>(freq, 20, _sampleRate / 2.00001);
      reso = std::clamp<double>(reso, 0., 0.98);

      if (_lastFreq == freq && _lastReso == reso)
         return;

      _g = std::tan(3.14159265359 * freq / _sampleRate);
      _k = 2 - 2 * reso;
      _a1 = 1 / (1 + _g * (_g + _k));
      _a2 = _g * _a1;
      _a3 = _g * _a2;

      _lastFreq = freq;
      _lastReso = reso;
   }

   clap_process_status SvfModule::process(const Context &c, uint32_t numFrames) noexcept {
      assert(_isActive);

      auto const in = _input->data();
      auto const inStride = _input->stride();
      auto const out = _output.data();
      _output.setConstant(false);

      auto &freqBuffer = _freqParam->modulatedValueBuffer();
      auto &resoBuffer = _resoParam->modulatedValueBuffer();
      auto &modeBuffer = _modeParam->modulatedValueBuffer();
      auto &keytrackBuffer = _keytrackParam->modulatedValueBuffer();
      auto &envAmountBuffer = _envParam->modulatedValueBuffer();
      auto &fmAmountBuffer = _fmParam->modulatedValueBuffer();

      if (_voiceModule) {
         _ktBuffer.product(keytrackBuffer, _voiceModule->pitch(), numFrames);
      } else {
         _ktBuffer.data()[0] = 0;
         _ktBuffer.setConstant(true);
      }

      if (_envInput) {
         _envBuffer.product(envAmountBuffer, *_envInput, numFrames);
      } else {
         _envBuffer.data()[0] = 0;
         _envBuffer.setConstant(true);
      }

      if (_fmInput) {
         _fmBuffer.product(fmAmountBuffer, *_fmInput, numFrames);
      } else {
         _fmBuffer.product(fmAmountBuffer, *_input, numFrames);
      }

      _fmBuffer.sum(_fmBuffer, _envBuffer, numFrames);
      _fmBuffer.sum(_fmBuffer, _ktBuffer, numFrames);

      for (uint32_t i = 0; i < numFrames; ++i) {
         double fmRatio = tuningToRatio(_fmBuffer.getSample(i));
         setFilter(freqBuffer.getSample(i) * fmRatio, resoBuffer.getSample(i));

         double v0 = in[i * inStride];
         double v3 = v0 - _ic2eq;
         double v1 = _ic1eq * _a1 + v3 * _a2;
         double v2 = _ic2eq + _ic1eq * _a2 + v3 * _a3;

         _ic1eq = v1 * 2.0 - _ic1eq;
         _ic2eq = v2 * 2.0 - _ic2eq;

         double modeValue = modeBuffer.getSample(i);
         Mode mode = static_cast<Mode>(std::clamp<int>(modeValue, 0, 2));

         switch (mode) {
         case Mode::LP:
            out[i] = v2;
            break;
         case Mode::BP:
            out[i] = v1;
            break;
         case Mode::HP:
            out[i] = v0 - _k * v1 - v2;
            break;
         default:
            assert(false);
            out[i] = 0;
            break;
         }
      }

      return CLAP_PROCESS_CONTINUE;
   }
} // namespace clap
