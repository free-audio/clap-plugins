#pragma once

#include <cstdint>
#include <cstdlib>
#include <new>

namespace clap {
   template <typename T = float>
   class AudioBuffer {
   public:
      AudioBuffer(uint32_t channelCount, uint32_t frameCount, double sampleRate = 0)
         : _channelCount(channelCount), _frameCount(frameCount), _sampleRate(sampleRate),
           _data(static_cast<T *>(std::aligned_alloc(32, channelCount * frameCount * sizeof(T)))) {
         if (!_data)
            throw std::bad_alloc();
      }

      AudioBuffer(const AudioBuffer<T> &other) = delete;
      AudioBuffer<T> &operator=(const AudioBuffer<T> &) = delete;
      AudioBuffer<T> &operator=(AudioBuffer<T> &&) = delete;
      AudioBuffer(AudioBuffer<T> &&o) = delete;

      ~AudioBuffer() { std::free(_data); }

      [[nodiscard]] T *data() noexcept { return _data; }
      [[nodiscard]] const T *data() const noexcept { return _data; }

      [[nodiscard]] uint32_t stride() const noexcept { return _stride; }
      [[nodiscard]] bool isConstant() const noexcept { return _stride == 0; }
      void setConstant(bool isConstant) noexcept { _stride = isConstant ? _channelCount : 0; }

      [[nodiscard]] uint32_t frameCount() const noexcept { return _frameCount; }

      [[nodiscard]] double sampleRate() const noexcept { return _sampleRate; }

   private:
      const uint32_t _channelCount;
      const uint32_t _frameCount;
      const double _sampleRate; // 0 if unspecified
      T *const _data;
      uint32_t _stride;
   };
} // namespace clap
