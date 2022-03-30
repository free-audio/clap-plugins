#pragma once

#include <cstdint>
#include <string>

#include <clap/clap.h>

#include "parameter.hh"
#include "voice.hh"
#include "audio-buffer.hh"

namespace clap {
   class CorePlugin;

   /** Re-usable processing unit */
   class Module {
   public:
      // This value is fixed and must **NEVER** change.
      // It defines the amount of space reserved for clap parameters id within
      // this module.
      static const constexpr uint32_t parameter_capacity = 1024;

      Module(const Module &) = delete;
      Module(Module &&) = delete;
      Module &operator=(const Module &) = delete;
      Module &operator=(Module &&) = delete;

      Module(CorePlugin &plugin, std::string name, clap_id paramIdStart);
      virtual ~Module();

      [[nodiscard]] virtual bool activate(double sampleRate, uint32_t maxFrameCount) {
         return true;
      }
      virtual void deactivate() {}

      /**
       * Process method.
       *
       * If the module is strictly monophonic, then shouldProcessVoices() should return false.
       * If the module is polyphonic, then process() will compute the monophonic parts of the
       * module, and processVoice() the voice specific parts.
       * \{
       */
      [[nodiscard]] virtual clap_process_status process(const clap_process *process) noexcept {
         return CLAP_PROCESS_SLEEP;
      }
      [[nodiscard]] virtual bool shouldProcessVoices() const noexcept { return false; }
      [[nodiscard]] virtual clap_process_status processVoice(uint32_t voiceIndex, const clap_process *process) noexcept {
         return CLAP_PROCESS_SLEEP;
      }
      /** \} */

   protected:
      [[nodiscard]] virtual bool wantsNoteEvents() const noexcept { return false; }
      virtual void onNoteOn(int32_t voiceIndex, const clap_event_note& note) noexcept {}
      virtual void onNoteOff(int32_t voiceIndex, const clap_event_note& note) noexcept {}
      virtual void onNoteChoke(int32_t voiceIndex, const clap_event_note& note) noexcept {}

      CorePlugin &_plugin;
      const std::string _name;
      const clap_id _paramIdStart;
   };
} // namespace clap
