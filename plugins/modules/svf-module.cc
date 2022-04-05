#include <algorithm>
#include <cmath>

#include "svf-module.hh"

namespace clap {

   SvfModule::SvfModule(CorePlugin &plugin, std::string name, uint32_t moduleId)
      : Module(plugin, name, moduleId) {
      char moduleName[CLAP_MODULE_SIZE];
      snprintf(moduleName, sizeof(moduleName), "/%s", _name.c_str());

      _freqParam = addParameter(
         0, "freq", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 20, 20000, 400);
      _resoParam =
         addParameter(1, "reso", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 0, 1, 0.3);

      _output.setConstant(false);
   }

   SvfModule::SvfModule(const SvfModule &m)
      : Module(m), _freqParam(m._freqParam), _resoParam(m._resoParam) {}

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

   clap_process_status SvfModule::process(Context &c, uint32_t numFrames) noexcept {
      assert(_isActive);

      auto const in = _input->data();
      auto const inStride = _input->stride();
      auto const out = _output.data();

      auto &freqBuffer = _freqParam->modulatedValueBuffer();
      auto &resoBuffer = _resoParam->modulatedValueBuffer();

      for (uint32_t i = 0; i < numFrames; ++i) {
         setFilter(freqBuffer.getSample(i, 0), resoBuffer.getSample(i, 0));

         double v0 = in[i * inStride];
         double v3 = v0 - _ic2eq;
         double v1 = _ic1eq * _a1 + v3 * _a2;
         double v2 = _ic2eq + _ic1eq * _a2 + v3 * _a3;

         _ic1eq = v1 * 2.0 - _ic1eq;
         _ic2eq = v2 * 2.0 - _ic2eq;

         if (true)
            out[i] = v2; // low pass
         else if (true)
            out[i] = v1; // band pass
         else
            out[i] = v0 - _k * v1 - v2; // high pass
      }

      return CLAP_PROCESS_CONTINUE;
   }
} // namespace clap
