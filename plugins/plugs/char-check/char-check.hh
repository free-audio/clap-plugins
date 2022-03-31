#pragma once

#include "../core-plugin.hh"

namespace clap {
   class CharCheck final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      CharCheck(const std::string &pluginPath, const clap_host *host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      void addDumbParam(const char *name);

   private:
      clap_id _nextParamId = 0;
   };
} // namespace clap
