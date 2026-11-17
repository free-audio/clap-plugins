#include <cstring>

#include "builtin-presets.hh"

namespace clap {
   namespace {
      // The gain plug-in's parameter, in decibels
      constexpr clap_id kGainParamId = 0;
      constexpr const char *kGainPluginId = "com.github.free-audio.clap.gain";
   } // namespace

   const std::vector<BuiltinPreset> &builtinPresets() {
      static const std::vector<BuiltinPreset> presets = {
         {kGainPluginId, "Unity", "gain/unity", {{kGainParamId, 0}}},
         {kGainPluginId, "Quiet", "gain/quiet", {{kGainParamId, -12}}},
         // A name with a dot in it, which is not a file extension
         {kGainPluginId, "Boost 6.5", "gain/boost", {{kGainParamId, 6.5}}},
      };
      return presets;
   }

   const BuiltinPreset *findBuiltinPreset(const char *pluginId, const char *loadKey) {
      for (auto &preset : builtinPresets())
         if (!std::strcmp(preset.pluginId, pluginId) && !std::strcmp(preset.loadKey, loadKey))
            return &preset;
      return nullptr;
   }
} // namespace clap
