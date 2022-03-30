#include "../core-plugin.hh"
#include "../modules/adsr-module.hh"

namespace clap {
   class PmOsc final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      PmOsc(const std::string& pluginPath, const clap_host *host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      bool init() noexcept override;
      void defineAudioPorts() noexcept;

      clap_process_status process(const clap_process *process) noexcept override;

   private:
        AdsrModule _ampAdsrModule;
   };
} // namespace clap
