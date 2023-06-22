#include <cstring>

#include "../../audio-buffer.hh"
#include "../../modules/voice-expander-module.hh"
#include "../../modules/module.hh"
#include "adsr-plug.hh"

namespace clap {

   class AdsrPlugModule final : public Module {
   public:
      AdsrPlugModule(AdsrPlug &AdsrPlug) : Module(AdsrPlug, "", 0), _adsr(AdsrPlug, "amp", 1) {}
      AdsrPlugModule(const AdsrPlugModule &m) : Module(m), _adsr(m._adsr) {}

      std::unique_ptr<Module> cloneVoice() const override
      {
         return std::make_unique<AdsrPlugModule>(*this);
      }

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override {
         bool succeed = true;
         succeed &= _adsr.activate(sampleRate, maxFrameCount, isRealTime);

         if (succeed)
            return true;

         deactivate();
         return false;
      }

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         auto status = _adsr.process(c, numFrames);
         c.audioOutputs[0]->copy(_adsr.outputBuffer(), numFrames);
         return status;
      }

      bool wantsNoteEvents() const noexcept override { return true; }
      void onNoteOn(const clap_event_note &note) noexcept override { _adsr.onNoteOn(note); }
      void onNoteOff(const clap_event_note &note) noexcept override { _adsr.onNoteOff(note); }
      void onNoteChoke(const clap_event_note &note) noexcept override { _adsr.onNoteChoke(note); }

   protected:
      AdsrModule _adsr;
   };

   const clap_plugin_descriptor *AdsrPlug::descriptor() {
      static const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.adsr",
         "CLAP ADSR",
         "clap",
         "https://github.com/free-audio/clap-plugins",
         nullptr,
         nullptr,
         "0.1",
         "Test plugin for CLAP ADSR",
         features};
      return &desc;
   }

   AdsrPlug::AdsrPlug(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "AdsrPlug"), descriptor(), host) {
      _rootModule = std::make_unique<AdsrPlugModule>(*this);
   }

   bool AdsrPlug::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      defineNotePorts();
      return true;
   }

   void AdsrPlug::defineNotePorts() noexcept {
      assert(!isActive());

      clap_note_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
      info.supported_dialects = CLAP_NOTE_DIALECT_CLAP;

      _noteInputs.clear();
      _noteInputs.push_back(info);
      _noteOutputs.clear();
   }

   void AdsrPlug::defineAudioPorts() noexcept {
      assert(!isActive());

      clap_audio_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.flags = CLAP_AUDIO_PORT_IS_MAIN;
      info.in_place_pair = CLAP_INVALID_ID;
      info.channel_count = 2;
      info.port_type = CLAP_PORT_STEREO;

      _audioInputs.clear();
      _audioOutputs.clear();
      _audioOutputs.push_back(info);
   }
} // namespace clap
