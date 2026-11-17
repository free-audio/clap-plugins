#pragma once

#include <clap/clap.h>

namespace clap {
   // One provider, declaring the presets built into this binary as a
   // CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN location.
   const clap_preset_discovery_factory *presetDiscoveryFactory();
} // namespace clap
