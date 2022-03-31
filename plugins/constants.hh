#pragma once

#include <cstdint>

namespace clap {
   static const constexpr uint32_t MAX_VOICES = 16;

   // Internal block size used to divide large process calls
   static const constexpr uint32_t BLOCK_SIZE = 256;
} // namespace clap