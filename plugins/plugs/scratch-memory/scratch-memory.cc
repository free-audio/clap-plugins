#include <cstring>

#include "scratch-memory.hh"

namespace clap {
   static constexpr size_t SCRATCH_SIZE = 16 * 1024;

   class ScratchMemoryModule final : public Module {
   public:
      ScratchMemoryModule(ScratchMemory &plugin) : Module(plugin, "", 0) {}

      [[nodiscard]] virtual bool
      doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override {
         auto &h = _plugin.host();

         _didReserveScratchMemory = false;
         if (h.canUseScratchMemory())
            _didReserveScratchMemory = h.scratchMemoryReserve(SCRATCH_SIZE, 0);

         return true;
      }

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         auto &h = _plugin.host();
         if (_didReserveScratchMemory) {
            auto scratchPtr = h.scratchMemoryAccess();
            if (scratchPtr)
               std::memset(scratchPtr, 0, SCRATCH_SIZE);
         }
         return CLAP_PROCESS_CONTINUE;
      }

      bool _didReserveScratchMemory = false;
   };

   const clap_plugin_descriptor *ScratchMemory::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_ANALYZER, nullptr};

      static const clap_plugin_descriptor desc = {CLAP_VERSION,
                                                  "com.github.free-audio.clap.scratch-memory-test",
                                                  "Scratch Memory Test",
                                                  "clap",
                                                  "https://github.com/free-audio/clap",
                                                  nullptr,
                                                  nullptr,
                                                  "1.0",
                                                  "Tests scratch memory",
                                                  features};

      return &desc;
   }

   enum {
      kParamIdOffset = 0,
   };

   ScratchMemory::ScratchMemory(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "transport-info"), descriptor(), host) {
      _rootModule = std::make_unique<ScratchMemoryModule>(*this);
   }
} // namespace clap
