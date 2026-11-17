#pragma once

#include <utility>
#include <vector>

#include <clap/clap.h>

namespace clap {
   // A preset stored in the plug-in binary rather than in a file. The preset discovery provider
   // lists them under a CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN location, and
   // clap_plugin_preset_load.from_location() loads one by its load key.
   struct BuiltinPreset {
      const char *pluginId;
      const char *name;

      // Unique within the whole binary, which is the container the location stands for.
      const char *loadKey;

      // Parameters left out keep their default value.
      std::vector<std::pair<clap_id, double>> values;
   };

   const std::vector<BuiltinPreset> &builtinPresets();

   const BuiltinPreset *findBuiltinPreset(const char *pluginId, const char *loadKey);
} // namespace clap
