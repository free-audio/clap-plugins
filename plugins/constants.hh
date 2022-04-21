#pragma once

#include <cstdint>

namespace clap {
   static const constexpr uint32_t MAX_VOICES = 8;

   // Internal block size used to divide large process calls
   static const constexpr uint32_t BLOCK_SIZE = 128;

   // Grow it when necessary
   static const constexpr uint32_t MAX_AUDIO_CHANNELS = 2;
} // namespace clap
