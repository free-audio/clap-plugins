#include <algorithm>
#include <cmath>
#include <vector>

#include "../parameter.hh"
#include "../tuning-utilities.hh"
#include "../value-types/enumerated-value-type.hh"
#include "../value-types/frequency-value-type.hh"
#include "../value-types/simple-value-type.hh"
#include "svf-module.hh"
#include "voice-module.hh"

namespace clap {

   template <uint32_t ChannelCount>
   SvfModule<ChannelCount>::SvfModule(CorePlugin &plugin, std::string name, uint32_t moduleId)
      : Module(plugin, name, moduleId) {
      _freqParam = addParameter(
         0, "freq", Parameter::POLY_FLAGS, std::make_unique<FrequencyValueType>(20, 20000, 20000));
      _resoParam = addParameter(
         1, "reso", Parameter::POLY_FLAGS, std::make_unique<SimpleValueType>(0, 0.98, 0));
      _modeParam = addParameter(
         2,
         "mode",
         CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_STEPPED,
         std::make_unique<EnumeratedValueType>(std::vector<std::string>{"LP", "BP", "HP"}, 0));
      _keytrackParam =
         addParameter(3, "kt", Parameter::POLY_FLAGS, std::make_unique<SimpleValueType>(0, 120, 0));
      _envParam = addParameter(
         4, "env", Parameter::POLY_FLAGS, std::make_unique<SimpleValueType>(-120, 120, 0));

      _fmParam = addParameter(
         5, "fm", Parameter::POLY_FLAGS, std::make_unique<SimpleValueType>(-120, 120, 0));

      _output.setConstant(false);
   }

   template <uint32_t ChannelCount>
   SvfModule<ChannelCount>::SvfModule(const SvfModule &m)
      : Module(m), _freqParam(m._freqParam), _resoParam(m._resoParam), _modeParam(m._modeParam),
        _keytrackParam(m._keytrackParam), _envParam(m._envParam), _fmParam(m._fmParam) {}

   template <uint32_t ChannelCount>
   SvfModule<ChannelCount>::~SvfModule() = default;

   template <uint32_t ChannelCount>
   std::unique_ptr<Module> SvfModule<ChannelCount>::cloneVoice() const {
      return std::make_unique<SvfModule>(*this);
   }

   template <uint32_t ChannelCount>
   bool
   SvfModule<ChannelCount>::doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) {
      _sampleRate = sampleRate;
      reset();
      return true;
   }

   template <uint32_t ChannelCount>
   void SvfModule<ChannelCount>::reset() noexcept {
      for (auto &ch : _channels) {
         ch._ic1eq = 0;
         ch._ic2eq = 0;
         ch._lastFreq = 0;
         ch._lastReso = 0;
      }
   }

   template <uint32_t ChannelCount>
   void SvfModule<ChannelCount>::setFilter(Channel &ch, double freq, double reso) {
      freq = std::clamp<double>(freq, 20, _sampleRate / 2.00001);
      reso = std::clamp<double>(reso, 0., 0.98);

      if (ch._lastFreq == freq && ch._lastReso == reso)
         return;

      ch._g = std::tan(3.14159265359 * freq / _sampleRate);
      ch._k = 2 - 2 * reso;
      ch._a1 = 1 / (1 + ch._g * (ch._g + ch._k));
      ch._a2 = ch._g * ch._a1;
      ch._a3 = ch._g * ch._a2;

      ch._lastFreq = freq;
      ch._lastReso = reso;
   }

   template <uint32_t ChannelCount>
   clap_process_status SvfModule<ChannelCount>::process(const Context &c,
                                                        uint32_t numFrames) noexcept {
      assert(_isActive);
      assert(_input->channelCount() == ChannelCount);

      auto const in = _input->data();
      auto const inStride = _input->stride();
      auto const out = _output.data();
      _output.setConstant(false);

      auto &freqBuffer = _freqParam->voiceBuffer(_voiceModule);
      auto &resoBuffer = _resoParam->voiceBuffer(_voiceModule);
      auto &modeBuffer = _modeParam->voiceBuffer(_voiceModule);
      auto &keytrackBuffer = _keytrackParam->voiceBuffer(_voiceModule);
      auto &envAmountBuffer = _envParam->voiceBuffer(_voiceModule);
      auto &fmAmountBuffer = _fmParam->voiceBuffer(_voiceModule);

      if (_voiceModule) {
         _ktBuffer.product(keytrackBuffer, _voiceModule->pitch(), numFrames);
      } else {
         _ktBuffer.setConstantValue(0);
      }

      if (_envInput) {
         _envBuffer.product(envAmountBuffer, *_envInput, numFrames);
      } else {
         _envBuffer.setConstantValue(0);
      }

      if (_fmInput) {
         _fmBuffer.product(fmAmountBuffer, *_fmInput, numFrames);
      } else {
         _fmBuffer.product(fmAmountBuffer, *_input, numFrames);
      }

      _fmBuffer.sum(_fmBuffer, _envBuffer, numFrames);
      _fmBuffer.sum(_fmBuffer, _ktBuffer, numFrames);

      for (uint32_t i = 0; i < numFrames; ++i) {
         double freq = freqBuffer.getSample(i);
         double reso = resoBuffer.getSample(i);
         double modeValue = modeBuffer.getSample(i);
         Mode mode = static_cast<Mode>(std::clamp<int>(modeValue, 0, 2));

         for (uint32_t channel = 0; channel < ChannelCount; ++channel) {
            auto &ch = _channels[channel];
            double fmRatio = tuningToRatio(_fmBuffer.getSample(i, channel));
            setFilter(ch, freq * fmRatio, reso);

            double v0 = 16 * std::tanh(in[i * inStride + channel] / 16.);
            double v3 = v0 - ch._ic2eq;
            double v1 = ch._ic1eq * ch._a1 + v3 * ch._a2;
            double v2 = ch._ic2eq + ch._ic1eq * ch._a2 + v3 * ch._a3;

            ch._ic1eq = v1 * 2.0 - ch._ic1eq;
            ch._ic2eq = v2 * 2.0 - ch._ic2eq;

            double &o = out[i * ChannelCount + channel];
            switch (mode) {
            case Mode::LP:
               o = v2;
               break;
            case Mode::BP:
               o = v1;
               break;
            case Mode::HP:
               o = v0 - ch._k * v1 - v2;
               break;
            default:
               assert(false);
               o = 0;
               break;
            }
         }
      }

      return CLAP_PROCESS_CONTINUE;
   }
} // namespace clap

template class clap::SvfModule<1>;
template class clap::SvfModule<2>;
