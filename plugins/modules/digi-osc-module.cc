#include "digi-osc-module.hh"
#include "voice-module.hh"

namespace clap {

   DigiOscModule::DigiOscModule(CorePlugin &plugin, std::string name, uint32_t moduleId)
      : Module(plugin, name, moduleId) {
      char moduleName[CLAP_MODULE_SIZE];
      snprintf(moduleName, sizeof(moduleName), "/%s", _name.c_str());

      _pdParam = addParameter(0,
                              "phase distortion",
                              CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE,
                              std::make_unique<SimpleValueType>(0, 1, 0));
   }

   DigiOscModule::DigiOscModule(const DigiOscModule &m) : Module(m), _pdParam(m._pdParam) {}

   DigiOscModule::~DigiOscModule() = default;

   std::unique_ptr<Module> DigiOscModule::cloneVoice() const {
      return std::make_unique<DigiOscModule>(*this);
   }

   bool DigiOscModule::doActivate(double sampleRate, uint32_t maxFrameCount) { return true; }

   clap_process_status DigiOscModule::process(const Context &c, uint32_t numFrames) noexcept {
      assert(_isActive);

      auto out = _buffer.data();
      _buffer.setConstant(false);

      auto &voiceTuningBuffer = _voiceModule->tuning();

      auto &pdBuffer = _pdParam->modulatedValueBuffer();

      for (uint32_t i = 0; i < numFrames; ++i) {
         double freq = 440; // TODO
         double phaseInc = freq * c.sampleRateInvD;
         out[i] = std::sin(2 * M_PI * _phase);
         _phase += phaseInc;
         _phase -= std::floor(_phase);
         assert(_phase >= 0);
      }

      return CLAP_PROCESS_CONTINUE;
   }
} // namespace clap
