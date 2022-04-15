#pragma once

#include <clap/clap.h>

namespace clap {
   [[nodiscard]] clap_process_status mergeProcessStatus(clap_process_status a,
                                                        clap_process_status b) noexcept;
} // namespace clap