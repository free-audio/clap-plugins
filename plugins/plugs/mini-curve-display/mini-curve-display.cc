#include <cstring>

#include "mini-curve-display.hh"

namespace clap {

   class MiniCurveDisplayModule final : public Module {
   public:
      MiniCurveDisplayModule(MiniCurveDisplay &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         c.audioOutputs[0]->copy(*c.audioInputs[0], numFrames);

         return CLAP_PROCESS_SLEEP;
      }
   };

   const clap_plugin_descriptor *MiniCurveDisplay::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.mini-curve-display",
         "Mini Curve Display",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "example mini curve display plugin",
         features};
      return &desc;
   }

   MiniCurveDisplay::MiniCurveDisplay(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "mini-curve-display"), descriptor(), host) {
      _rootModule = std::make_unique<MiniCurveDisplayModule>(*this);
   }

   bool MiniCurveDisplay::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void MiniCurveDisplay::defineAudioPorts() noexcept {
      assert(!isActive());

      _channelCount = trackChannelCount();

      clap_audio_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.flags = CLAP_AUDIO_PORT_IS_MAIN;
      info.in_place_pair = 0;
      info.channel_count = _channelCount;
      info.port_type = nullptr;

      _audioInputs.clear();
      _audioInputs.push_back(info);
      _audioOutputs.clear();
      _audioOutputs.push_back(info);
   }

   bool MiniCurveDisplay::implementsMiniCurveDisplay() const noexcept { return true; }

   bool MiniCurveDisplay::miniCurveDisplayRender(uint16_t *data, uint32_t data_size) noexcept {
      const double k = (2.0 * M_PI) / data_size;
      for (uint32_t i = 0; i < data_size; ++i) {
         const double phase = i * k;
         data[i] = (std::sin(phase) + 1) * std::numeric_limits<uint16_t>::max() / 2;
      }
      return true;
   }

   void MiniCurveDisplay::miniCurveDisplaySetObserved(bool is_observed) noexcept {
      _isObserved = true;
   }

   bool MiniCurveDisplay::miniCurveDisplayGetAxisName(char *x_name,
                                                      char *y_name,
                                                      uint32_t name_capacity) noexcept {
      if (x_name)
         snprintf(x_name, name_capacity, "x");
      if (y_name)
         snprintf(y_name, name_capacity, "x");
      return true;
   }
} // namespace clap
