#include "adsr-module.hh"

namespace clap {

   AdsrModule::AdsrModule(CorePlugin &plugin, std::string name, uint32_t moduleId)
      : Module(plugin, name, moduleId) {
      char moduleName[CLAP_MODULE_SIZE];
      snprintf(moduleName, sizeof(moduleName), "/%s", _name.c_str());

      _attackParam = addParameter(
         0, "attack", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 0, 1, 0.1);
      _decayParam =
         addParameter(1, "decay", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 0, 1, 0.2);
      _sustainParam = addParameter(
         2, "sustain", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 0, 1, 0.8);
      _releaseParam = addParameter(
         3, "release", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 0, 1, 0.3);
      _velocityParam = addParameter(
         4, "velocity", CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE, 0, 1, 0.3);
   }

   AdsrModule::AdsrModule(const AdsrModule &m)
      : Module(m), _attackParam(m._attackParam), _decayParam(m._decayParam),
        _sustainParam(m._sustainParam), _releaseParam(m._releaseParam),
        _velocityParam(m._velocityParam) {}

   AdsrModule::~AdsrModule() = default;

   std::unique_ptr<Module> AdsrModule::cloneVoice() const {
      return std::make_unique<AdsrModule>(*this);
   }

   void AdsrModule::trigger(double velocity) {
      _phase = Phase::Attack;
      _velocity = velocity;
      // TODO: use velocity
   }

   void AdsrModule::release() {
      switch (_phase) {
      case Phase::Attack:
      case Phase::Decay:
      case Phase::Sustain:
         _phase = Phase::Release;
         return;

      case Phase::Choke:
      case Phase::Rest:
      case Phase::Release:
         return;
      }
   }

   void AdsrModule::choke() {
      switch (_phase) {
      case Phase::Attack:
      case Phase::Decay:
      case Phase::Sustain:
         _phase = Phase::Release;
         return;

      case Phase::Choke:
      case Phase::Rest:
      case Phase::Release:
         return;
      }
   }

   bool AdsrModule::wantsNoteEvents() const noexcept { return true; }

   void AdsrModule::onNoteOn(const clap_event_note &note) noexcept { trigger(note.velocity); }

   void AdsrModule::onNoteOff(const clap_event_note &note) noexcept { release(); }

   void AdsrModule::onNoteChoke(const clap_event_note &note) noexcept { choke(); }

   clap_process_status AdsrModule::process(Context &c, uint32_t numFrames) noexcept {
      assert(_isActive);

      switch (_phase) {
      case Phase::Rest:
         break;
      case Phase::Attack:
         break;
      case Phase::Decay:
         break;
      case Phase::Sustain:
         break;
      case Phase::Release:
         break;
      case Phase::Choke:
         break;
      }

      return _phase == Phase::Rest ? CLAP_PROCESS_SLEEP : CLAP_PROCESS_CONTINUE;
   }

} // namespace clap
