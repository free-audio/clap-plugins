#include "root-module.hh"
#include "../core-plugin.hh"

namespace clap {
   void RootModule::addModule(Module *module) {
      _modules.push_back(module);
      if (module->wantsNoteEvents())
         _noteListeners.push_back(module);
   }

   clap_process_status RootModule::process(const clap_process *process) noexcept {

      const uint32_t evCount = process->in_events->size(process->in_events);
      uint32_t nextEvIndex = 0;
      uint32_t N = process->frames_count;

      // TODO: _plugin.processGuiEvents(process);

      /* foreach frames */
      for (uint32_t i = 0; i < process->frames_count;) {

         N = processEvents(process, nextEvIndex, evCount, i);

         /* Process audio until the next event */
         for (auto m : _modules) {
            m->process(process, i, N);
         }
      }

      // TODO: _plugin._pluginToGuiQueue.producerDone();
      return CLAP_PROCESS_CONTINUE_IF_NOT_QUIET;
   }

   uint32_t RootModule::processEvents(const clap_process *process,
                                      uint32_t &index,
                                      uint32_t count,
                                      uint32_t time) {
      for (; index < count; ++index) {
         auto hdr = process->in_events->get(process->in_events, index);

         if (hdr->time < time) {
            _plugin.hostMisbehaving("Events must be ordered by time");
            std::terminate();
         }

         if (hdr->time > time) {
            // This event is in the future
            return std::min(hdr->time, process->frames_count);
         }

         if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
            switch (hdr->type) {
            case CLAP_EVENT_PARAM_VALUE:
            case CLAP_EVENT_PARAM_MOD:
               processInputParameterChange(hdr);
               break;
            }
         }
      }

      return process->frames_count;
   }
} // namespace clap
