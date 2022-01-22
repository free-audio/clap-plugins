#pragma once

#ifdef __APPLE__
#   include "cf-timer.hh"

namespace clap {
   using NativeTimer = CFTimer;
}

#endif