#include "adsr-module.hh"

namespace clap {

   AdsrModule::AdsrModule(CorePlugin &plugin, std::string name, clap_id paramIdStart)
      : Module(plugin, name, paramIdStart) {}

   AdsrModule::~AdsrModule() = default;

   void AdsrModule::trigger(int32_t voiceIndex, double velocity) {
      _states[voiceIndex].phase = State::Attack;
      // TODO: use velocity
   }

   void AdsrModule::release(int32_t voiceIndex) {
      switch (_states[voiceIndex].phase) {
      case State::Attack:
      case State::Decay:
      case State::Sustain:
         _states[voiceIndex].phase = State::Release;
         return;

      case State::Choke:
      case State::Rest:
      case State::Release:
         return;
      }
   }

   void AdsrModule::choke(int32_t voiceIndex) {
      switch (_states[voiceIndex].phase) {
      case State::Attack:
      case State::Decay:
      case State::Sustain:
         _states[voiceIndex].phase = State::Release;
         return;

      case State::Choke:
      case State::Rest:
      case State::Release:
         return;
      }
   }

   bool AdsrModule::wantsNoteEvents() const noexcept { return true; }

   void AdsrModule::onNoteOn(int32_t voiceIndex, const clap_event_note &note) noexcept {
      trigger(voiceIndex, note.velocity);
   }

   void AdsrModule::onNoteOff(int32_t voiceIndex, const clap_event_note &note) noexcept {
      release(voiceIndex);
   }

   void AdsrModule::onNoteChoke(int32_t voiceIndex, const clap_event_note &note) noexcept {
      choke(voiceIndex);
   }
} // namespace clap
