#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class ProjectLocation final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      ProjectLocation(const std::string &pluginPath, const clap_host &host);

      bool implementsProjectLocation() const noexcept override { return true; }
      void projectLocationSet(const clap_project_location_element *path,
                              uint32_t num_elements) noexcept override;

      static const clap_plugin_descriptor *descriptor();
   };
} // namespace clap