#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <clap/clap.h>

#include "../audio-buffer.hh"
#include "../context.hh"

namespace clap {
   class CorePlugin;
   class Parameter;
   class ValueType;
   class VoiceModule;
   class VoiceExpanderModule;

   /** Re-usable processing unit */
   class Module {
      friend class VoiceModule;

   public:
      // This value is fixed and must **NEVER** change.
      // It defines the amount of space reserved for clap parameters id within
      // this module.
      static const constexpr uint32_t parameter_capacity = 1024;

      Module(Module &&) = delete;
      Module &operator=(const Module &) = delete;
      Module &operator=(Module &&) = delete;

      Module(CorePlugin &plugin, std::string name, uint32_t moduleId);
      Module(const Module &) = default;
      virtual ~Module();

      [[nodiscard]] virtual std::unique_ptr<Module> cloneVoice() const;

      bool activate(double sampleRate, uint32_t maxFrameCount, bool isRealTime);
      [[nodiscard]] virtual bool
      doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime);
      void deactivate();
      virtual void doDeactivate();
      [[nodiscard]] bool isActive() const noexcept { return _isActive; }

      virtual bool startProcessing() noexcept { return true; }
      virtual void stopProcessing() noexcept {}
      virtual void reset() noexcept {}

      virtual clap_process_status process(const Context &c, uint32_t numFrames) noexcept {
         assert(_isActive);
         return CLAP_PROCESS_SLEEP;
      }

      [[nodiscard]] virtual bool wantsNoteEvents() const noexcept { return false; }
      virtual void onNoteOn(const clap_event_note &note) noexcept {}
      virtual void onNoteOff(const clap_event_note &note) noexcept {}
      virtual void onNoteChoke(const clap_event_note &note) noexcept {}
      virtual void onNoteExpression(const clap_event_note_expression &noteExp) noexcept {}

      // Sets a pointer to the parent voice module in order to retrive
      // the current voice info
      virtual void setVoiceModule(const VoiceModule *voiceModule) noexcept {
         _voiceModule = voiceModule;
      }

      virtual VoiceExpanderModule *getVoiceExpander() noexcept { return nullptr; }

      virtual const VoiceExpanderModule *getVoiceExpander() const noexcept { return nullptr; }

      virtual uint32_t latency() const noexcept { return 0; }

   protected:
      // Registers all parameters from this modules
      Parameter *addParameter(uint32_t id,
                              const std::string &name,
                              uint32_t flags,
                              std::unique_ptr<ValueType> valueType);

      CorePlugin &_plugin;
      const std::string _name;
      const uint32_t _moduleId;
      const clap_id _paramIdStart;

      const VoiceModule *_voiceModule = nullptr;

      bool _isActive = false;
   };
} // namespace clap
