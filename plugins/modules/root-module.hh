#pragma once

#include "module.hh"
#include "../parameters.hh"

namespace clap {
   /** Root module.
    * Converts external I/O to internal representation (audio buffer, ...).
    */
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

      [[nodiscard]] clap_process_status
      processRange(const clap_process *process, uint32_t frameOffset, uint32_t numFrames) noexcept;

   private:
      VoiceMode _voiceMode = VoiceMode::Mono;
      std::vector<Module *> _modules;
      std::vector<Module *> _noteListeners;
   };
} // namespace clap
