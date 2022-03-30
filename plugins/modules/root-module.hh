#pragma once

#include "../module.hh"

namespace clap {
   class Voice;

   class RootModule final : public Module {
   public:
      RootModule(CorePlugin &plugin, std::string name, clap_id paramIdStart);
      ~RootModule() override;

      /**
       * Adds a module to the root module.
       * Modules are processed in the FIFO order.
       */
      void addModule(Module *module);

      enum class VoiceMode {
         /// Retriggers on each notes, single voice
         Mono,

         /// Don't retrigger unless in release phase, single voice
         Legato,

         /// One voice per-note
         Poly,
      };
      void setVoiceMode(VoiceMode mode);

      clap_process_status process(const clap_process *process) noexcept override;

   private:
      uint32_t processEvents(const clap_process *process,
                             uint32_t &index,
                             uint32_t count,
                             uint32_t time) noexcept;

      VoiceMode _voiceMode = VoiceMode::Mono;
      std::vector<Module *> _modules;
      std::vector<Module *> _noteListeners;
   };
} // namespace clap
