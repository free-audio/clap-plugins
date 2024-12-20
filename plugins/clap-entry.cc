#include <clap/clap.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

#include "plugs/adsr/adsr-plug.hh"
#include "plugs/char-check/char-check.hh"
#include "plugs/dc-offset/dc-offset.hh"
#include "plugs/gain/gain.hh"
#include "plugs/latency/latency.hh"
#include "plugs/offline-latency/offline-latency.hh"
#include "plugs/realtime-requirement/realtime-requirement.hh"
#include "plugs/svf/svf-plug.hh"
#include "plugs/synth/synth.hh"
#include "plugs/track-info/track-info.hh"
#include "plugs/transport/transport-info.hh"
#include "plugs/undo-test/undo-test.hxx"
#include "plugs/scratch-memory/scratch-memory.hh"

struct PluginEntry {
   using create_func = std::function<const clap_plugin *(const clap_host &)>;

   PluginEntry(const clap_plugin_descriptor *d, create_func &&func)
      : desc(d), create(std::move(func)) {}

   const clap_plugin_descriptor *desc;
   std::function<const clap_plugin *(const clap_host &)> create;
};

static std::vector<PluginEntry> g_plugins;
static std::string g_pluginPath;

template <class T>
static void addPlugin() {
   g_plugins.emplace_back(T::descriptor(), [](const clap_host &host) -> const clap_plugin * {
      auto plugin = new T(g_pluginPath, host);
      return plugin->clapPlugin();
   });
}

static bool clap_init(const char *plugin_path) {
   g_pluginPath = plugin_path;

   addPlugin<clap::Synth>();
   addPlugin<clap::DcOffset>();
   addPlugin<clap::TransportInfo>();
   addPlugin<clap::Gain>();
   addPlugin<clap::CharCheck>();
   addPlugin<clap::AdsrPlug>();
   addPlugin<clap::SvfPlug>();
   addPlugin<clap::Latency>();
   addPlugin<clap::OfflineLatency>();
   addPlugin<clap::RealtimeRequirement>();
   addPlugin<clap::TrackInfo>();
   addPlugin<clap::UndoTest<true, true>>();
   addPlugin<clap::UndoTest<true, false>>();
   addPlugin<clap::UndoTest<false, false>>();
   addPlugin<clap::ScratchMemory>();
   return true;
}

static void clap_deinit(void) {
   g_plugins.clear();
   g_pluginPath.clear();
}

static std::mutex g_entry_init_guard;
static int g_entry_init_counter = 0;

static bool clap_init_guard(const char *plugin_path) {
   std::lock_guard<std::mutex> guard(g_entry_init_guard);
   const int cnt = ++g_entry_init_counter;
   if (cnt > 1)
      return true;
   if (clap_init(plugin_path))
      return true;
   g_entry_init_counter = 0;
   return false;
}

static void clap_deinit_guard(void) {
   std::lock_guard<std::mutex> guard(g_entry_init_guard);
   const int cnt = --g_entry_init_counter;
   if (cnt == 0)
      clap_deinit();
}

static uint32_t clap_get_plugin_count(const clap_plugin_factory *) { return g_plugins.size(); }

static const clap_plugin_descriptor *clap_get_plugin_descriptor(const clap_plugin_factory *,
                                                                uint32_t index) {
   if (index < 0 || index >= g_plugins.size()) {
      std::cerr << "index out of bounds: " << index << " not in 0.." << g_plugins.size()
                << std::endl;
      std::terminate();
   }

   return g_plugins[index].desc;
}

static const clap_plugin *
clap_create_plugin(const clap_plugin_factory *, const clap_host *host, const char *plugin_id) {
   for (auto &entry : g_plugins)
      if (!strcmp(entry.desc->id, plugin_id))
         return entry.create(*host);
   return nullptr;
}

static const clap_plugin_factory g_clap_plugin_factory = {
   clap_get_plugin_count,
   clap_get_plugin_descriptor,
   clap_create_plugin,
};

const void *clap_get_factory(const char *factory_id) {
   if (!::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
      return &g_clap_plugin_factory;
   return nullptr;
}

CLAP_EXPORT const clap_plugin_entry clap_entry = {
   CLAP_VERSION,
   clap_init_guard,
   clap_deinit_guard,
   clap_get_factory,
};
