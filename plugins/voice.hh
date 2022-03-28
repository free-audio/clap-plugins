#pragma once

#include <cstdint>

namespace clap {
   class Voice {
   public:
      [[nodiscard]] uint32_t voiceIndex() const noexcept;
      [[nodiscard]] bool isAssigned() const noexcept;

      // key and channel info
      [[nodiscard]] int32_t channel() const noexcept;
      [[nodiscard]] int32_t key() const noexcept;
      [[nodiscard]] int32_t lowestKey() const noexcept;
      [[nodiscard]] int32_t highestKey() const noexcept;
      [[nodiscard]] int32_t startKey() const noexcept;

      // note expressions for this voice
      [[nodiscard]] double pitch() const noexcept;
      [[nodiscard]] double brightness() const noexcept;
      [[nodiscard]] double pressure() const noexcept;
      [[nodiscard]] double vibrato() const noexcept;
      [[nodiscard]] double expression() const noexcept;
      [[nodiscard]] double velocity() const noexcept;
      [[nodiscard]] double gain() const noexcept;
      [[nodiscard]] double pan() const noexcept;

   private:
      const uint32_t _voiceIndex;
      bool _isAssigned = false;
   };
} // namespace clap