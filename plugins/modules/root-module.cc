#include "root-module.hh"
#include "../core-plugin.hh"

namespace clap {
   void RootModule::addModule(Module *module) {
      _modules.push_back(module);
      if (module->wantsNoteEvents())
         _noteListeners.push_back(module);
   }
} // namespace clap
