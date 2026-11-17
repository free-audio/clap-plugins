#include <cstring>

#include "builtin-presets.hh"
#include "preset-discovery-factory.hh"

namespace clap {
   namespace {
      const clap_preset_discovery_provider_descriptor kProviderDescriptor = {
         CLAP_VERSION,
         "com.github.free-audio.clap.builtin-presets",
         "clap-plugins built-in presets",
         "free-audio",
      };

      struct Provider {
         clap_preset_discovery_provider provider;
         const clap_preset_discovery_indexer *indexer;
      };

      Provider &from(const clap_preset_discovery_provider *provider) {
         return *static_cast<Provider *>(provider->provider_data);
      }

      bool providerInit(const clap_preset_discovery_provider *provider) {
         auto &self = from(provider);

         // A plug-in location has no path: the binary itself is the container.
         const clap_preset_discovery_location location = {
            CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT,
            "clap-plugins",
            CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN,
            nullptr,
         };
         return self.indexer->declare_location(self.indexer, &location);
      }

      void providerDestroy(const clap_preset_discovery_provider *provider) {
         delete &from(provider);
      }

      bool providerGetMetadata(const clap_preset_discovery_provider *provider,
                               uint32_t location_kind,
                               const char *location,
                               const clap_preset_discovery_metadata_receiver *receiver) {
         if (location_kind != CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN || location) {
            receiver->on_error(receiver, 0, "Only the presets built into the plug-in are provided");
            return false;
         }

         for (auto &preset : builtinPresets()) {
            if (!receiver->begin_preset(receiver, preset.name, preset.loadKey))
               return true;

            const clap_universal_plugin_id pluginId = {"clap", preset.pluginId};
            receiver->add_plugin_id(receiver, &pluginId);
            receiver->set_flags(receiver, CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT);
         }
         return true;
      }

      const void *providerGetExtension(const clap_preset_discovery_provider *provider,
                                       const char *extension_id) {
         return nullptr;
      }

      uint32_t factoryCount(const clap_preset_discovery_factory *factory) { return 1; }

      const clap_preset_discovery_provider_descriptor *
      factoryGetDescriptor(const clap_preset_discovery_factory *factory, uint32_t index) {
         return index == 0 ? &kProviderDescriptor : nullptr;
      }

      const clap_preset_discovery_provider *
      factoryCreate(const clap_preset_discovery_factory *factory,
                    const clap_preset_discovery_indexer *indexer,
                    const char *provider_id) {
         if (std::strcmp(provider_id, kProviderDescriptor.id))
            return nullptr;

         auto *self = new Provider;
         self->indexer = indexer;
         self->provider = {
            &kProviderDescriptor,
            self,
            providerInit,
            providerDestroy,
            providerGetMetadata,
            providerGetExtension,
         };
         return &self->provider;
      }

      const clap_preset_discovery_factory kFactory = {
         factoryCount,
         factoryGetDescriptor,
         factoryCreate,
      };
   } // namespace

   const clap_preset_discovery_factory *presetDiscoveryFactory() { return &kFactory; }
} // namespace clap
