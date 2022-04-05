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
      _noteOnVelocity = velocity;
      _state = 1 - _level / ExpCoeff::K;
      // TODO: use velocity
   }

   void AdsrModule::computeStateForDecay() {
      _state = 1 + ExpCoeff::thr + (_level - 1) / ExpCoeff::K;
   }

   void AdsrModule::release() {
      switch (_phase) {
      case Phase::Attack:
      case Phase::Decay:
      case Phase::Sustain:
         _phase = Phase::Release;
         computeStateForDecay();
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
         _phase = Phase::Choke;
         _state = 1; // TODO
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

   bool AdsrModule::doActivate(double sampleRate, uint32_t maxFrameCount) {
      _conv = DomainConverter<ExpCoeff>(ExpCoeff(sampleRate), 0);
      return true;
   }

   clap_process_status AdsrModule::process(Context &c, uint32_t numFrames) noexcept {
      assert(_isActive);

      auto out = _buffer.data();

      auto &attackBuffer = _attackParam->modulatedValueBuffer();
      auto &decayBuffer = _decayParam->modulatedValueBuffer();
      auto &sustainBuffer = _sustainParam->modulatedValueBuffer();
      auto &releaseBuffer = _releaseParam->modulatedValueBuffer();
      // auto &velocityBuffer = _velocityParam->modulatedValueBuffer();

      if (_phase == Phase::Rest) {
         _buffer.setConstant(true);
         out[0] = 0;
         return CLAP_PROCESS_SLEEP;
      } else if (_phase == Phase::Sustain && sustainBuffer.isConstant()) {
         _buffer.setConstant(true);
         out[0] = _level;
         return CLAP_PROCESS_CONTINUE;
      }

      _buffer.setConstant(false);

      for (uint32_t i = 0; i < numFrames; ++i) {
         switch (_phase) {
         case Phase::Rest:
            for (; i < numFrames; ++i)
               out[i] = 0;
            break;

         case Phase::Attack: {
            for (; i < numFrames; ++i) {
               double k = _conv.convert(attackBuffer.getSample(i, 0));
               _state *= k;
               _level = ExpCoeff::K * (1 - _state);

               if (_level >= 1) [[unlikely]] {
                  computeStateForDecay();
                  _phase = Phase::Decay;
                  _level = 1;
                  out[i] = 1;
                  break;
               }

               out[i] = _level;
            }
            break;
         }

         case Phase::Decay: {
            for (; i < numFrames; ++i) {
               double k = _conv.convert(decayBuffer.getSample(i, 0));
               auto s = sustainBuffer.getSample(i, 0);
               _state *= k;
               _level = 1 - ExpCoeff::K * (1 + ExpCoeff::thr - _state);
               out[i] = _level;

               if (_level <= s) [[unlikely]] {
                  _phase = Phase::Sustain;
                  _level = std::max(0., _level);
                  out[i] = _level;
                  break;
               }
            }
            break;
         }

         case Phase::Sustain: {
            for (; i < numFrames; ++i)
               out[i] = _level;
            break;
         }

         case Phase::Release: {
            for (; i < numFrames; ++i) {
               double k = _conv.convert(releaseBuffer.getSample(i, 0));
               _state *= k;

               _level = 1 - ExpCoeff::K * (1 + ExpCoeff::thr - _state);
               if (_level <= 0) [[unlikely]] {
                  _level = 0;
                  _phase = Phase::Rest;
                  out[i] = 0;
                  break;
               }

               out[i] = _level;
            }
            break;
         }

         case Phase::Choke:
            // TODO
            _phase = Phase::Rest;
            out[i] = 0;
            break;
         }
      }

      return _phase == Phase::Rest ? CLAP_PROCESS_SLEEP : CLAP_PROCESS_CONTINUE;
   }
} // namespace clap
