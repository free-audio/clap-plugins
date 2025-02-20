#include <chrono>
#include <cstring>

#include "mini-curve-display.hh"

#include "../../value-types/boolean-value-type.hh"

namespace clap {

   static const auto T0 = std::chrono::system_clock::now();

   template <bool IsDynamic>
   class MiniCurveDisplayModule final : public Module {
   public:
      MiniCurveDisplayModule(MiniCurveDisplay<IsDynamic> &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         c.audioOutputs[0]->copy(*c.audioInputs[0], numFrames);

         return CLAP_PROCESS_SLEEP;
      }
   };

   template <bool IsDynamic>
   const clap_plugin_descriptor *MiniCurveDisplay<IsDynamic>::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         IsDynamic ? "com.github.free-audio.clap.mini-curve-display-dynamic"
                   : "com.github.free-audio.clap.mini-curve-display",
         IsDynamic ? "Mini Curve Display (Dynamic)" : "Mini Curve Display",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "example mini curve display plugin",
         features};
      return &desc;
   }

   template <bool IsDynamic>
   MiniCurveDisplay<IsDynamic>::MiniCurveDisplay(const std::string &pluginPath,
                                                 const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "mini-curve-display"), descriptor(), host) {
      _rootModule = std::make_unique<MiniCurveDisplayModule<IsDynamic>>(*this);
   }

   template <bool IsDynamic>
   bool MiniCurveDisplay<IsDynamic>::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();

      if (_host.canUseMiniCurveDisplay())
         _host.miniCurveDisplaySetDynamic(IsDynamic);

      return true;
   }

   template <bool IsDynamic>
   void MiniCurveDisplay<IsDynamic>::defineAudioPorts() noexcept {
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

   template <bool IsDynamic>
   bool MiniCurveDisplay<IsDynamic>::implementsMiniCurveDisplay() const noexcept {
      return true;
   }

   template <bool IsDynamic>
   bool MiniCurveDisplay<IsDynamic>::miniCurveDisplayRender(uint16_t *data,
                                                            uint32_t data_size) noexcept {
      double phaseOffset = 0;
      if (IsDynamic) {
         const auto now = std::chrono::system_clock::now();
         const auto d = now.time_since_epoch();
         const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch() % std::chrono::seconds(4));
         phaseOffset = 2.0 * M_PI * ms.count() / 4000.0;
      }

      const double k = (2.0 * M_PI) / data_size;
      for (uint32_t i = 0; i < data_size; ++i) {
         const double phase = i * k + phaseOffset;
         data[i] = (std::sin(phase) + 1) * std::numeric_limits<uint16_t>::max() / 2;
      }
      return true;
   }

   template <bool IsDynamic>
   void MiniCurveDisplay<IsDynamic>::miniCurveDisplaySetObserved(bool is_observed) noexcept {
      _isObserved = true;
   }

   template <bool IsDynamic>
   bool MiniCurveDisplay<IsDynamic>::miniCurveDisplayGetAxisName(char *x_name,
                                                                 char *y_name,
                                                                 uint32_t name_capacity) noexcept {
      if (x_name)
         snprintf(x_name, name_capacity, "x");
      if (y_name)
         snprintf(y_name, name_capacity, "x");
      return true;
   }
} // namespace clap

template class clap::MiniCurveDisplay<false>;
template class clap::MiniCurveDisplay<true>;