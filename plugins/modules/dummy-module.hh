#pragma once

#include "module.hh"

namespace clap {
   class DummyModule : public Module {
   public:
      DummyModule(CorePlugin &plugin) : Module(plugin, "", 0) {}
   };
} // namespace clap