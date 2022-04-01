#pragma once

#include <array>
#include <memory>

#include "../constants.hh"
#include "../intrusive-list.hh"
#include "../module.hh"

#include "voice-module.hh"

namespace clap {
   class VoiceExpanderModule : public Module {
      VoiceExpanderModule(CorePlugin &plugin, clap_id paramIdStart, std::unique_ptr<Module> module);

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override;

      void doDeactivate() override;

      clap_process_status process(Context &c, uint32_t numFrames) noexcept override;

   private:
      IntrusiveList _activeVoices;
      std::array<std::unique_ptr<VoiceModule>, MAX_VOICES> _voices;
   };
} // namespace clap
