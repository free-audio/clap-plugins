#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class ScratchMemory final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      ScratchMemory(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();
   };
} // namespace clap