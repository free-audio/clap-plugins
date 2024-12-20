#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class Location final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      Location(const std::string &pluginPath, const clap_host &host);

      bool implementsLocation() const noexcept override { return true; }
      void locationSetLocation(const clap_plugin_location_element_t *path,
                               uint32_t num_elements) noexcept override;

      static const clap_plugin_descriptor *descriptor();
   };
} // namespace clap