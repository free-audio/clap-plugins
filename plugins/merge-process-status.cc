#include "merge-process-status.hh"

namespace clap {
   clap_process_status mergeProcessStatus(clap_process_status a, clap_process_status b) noexcept {
      if (a == CLAP_PROCESS_ERROR || b == CLAP_PROCESS_ERROR)
         return CLAP_PROCESS_ERROR;

      if (a == CLAP_PROCESS_CONTINUE || b == CLAP_PROCESS_CONTINUE)
         return CLAP_PROCESS_CONTINUE;

      if (a == CLAP_PROCESS_CONTINUE_IF_NOT_QUIET || b == CLAP_PROCESS_CONTINUE_IF_NOT_QUIET)
         return CLAP_PROCESS_CONTINUE_IF_NOT_QUIET;

      if (a == CLAP_PROCESS_TAIL || b == CLAP_PROCESS_TAIL)
         return CLAP_PROCESS_TAIL;

      return CLAP_PROCESS_SLEEP;
   }
} // namespace clap