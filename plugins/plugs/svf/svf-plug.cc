#include <cstring>

#include "../../audio-buffer.hh"
#include "../modules/svf-module.hh"
#include "svf-plug.hh"

namespace clap {

   class SvfPlugModule final : public Module {
   public:
      SvfPlugModule(SvfPlug &SvfPlug) : Module(SvfPlug, "", 0), _svf(SvfPlug, "amp", 1) {}
      SvfPlugModule(const SvfPlugModule &m) : Module(m), _svf(m._svf) {}

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override {
         bool succeed = true;
         succeed &= _svf.activate(sampleRate, maxFrameCount);

         if (succeed)
            return true;

         deactivate();
         return false;
      }

      clap_process_status process(Context &c, uint32_t numFrames) noexcept override {
         _svf.setInput(c.audioInputs[0].get());
         auto status = _svf.process(c, numFrames);
         c.audioOutputs[0]->copy(_svf.outputBuffer(), numFrames);
         return status;
      }

   protected:
      SvfModule _svf;
   };

   const clap_plugin_descriptor *SvfPlug::descriptor() {
      static const char *features[] = {"audio_effect", "filter", nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.svf",
         "CLAP SVF",
         "clap",
         "https://github.com/free-audio/clap-plugins",
         nullptr,
         nullptr,
         "0.1",
         "Test plugin for CLAP SVF",
         features};
      return &desc;
   }

   SvfPlug::SvfPlug(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "svf"), descriptor(), host) {
      _rootModule = std::make_unique<SvfPlugModule>(*this);
   }

   bool SvfPlug::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void SvfPlug::defineAudioPorts() noexcept {
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
      _audioInputs.push_back(info);
      _audioOutputs.push_back(info);
   }
} // namespace clap
