#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <new>

#include <clap/clap.h>

#include "constants.hh"

namespace clap {
   template <typename T = float>
   class AudioBuffer {
   public:
      explicit AudioBuffer(uint32_t channelCount = 1,
                           uint32_t frameCount = BLOCK_SIZE,
                           double sampleRate = 0)
         : _channelCount(channelCount), _frameCount(frameCount), _sampleRate(sampleRate),
           _data(static_cast<T *>(std::aligned_alloc(32, channelCount * frameCount * sizeof(T)))) {
         if (_channelCount > MAX_AUDIO_CHANNELS)
            throw std::invalid_argument("too many audio channels");

         if (!_data) [[unlikely]]
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
      void setConstantValue(double value) noexcept {
         _stride = 0;
         for (uint32_t i = 0; i < _channelCount; ++i)
            _data[i] = value;
      }
      void setConstant(bool isConstant) noexcept { _stride = _channelCount * !isConstant; }
      [[nodiscard]] const T getSample(uint32_t frame, uint32_t channel = 0) const noexcept {
         return _data[frame * _stride + channel];
      }

      [[nodiscard]] uint32_t frameCount() const noexcept { return _frameCount; }
      [[nodiscard]] uint32_t channelCount() const noexcept { return _channelCount; }

      [[nodiscard]] double sampleRate() const noexcept { return _sampleRate; }

      void
      fromClap(const clap_audio_buffer *buffer, uint32_t frameOffset, uint32_t frameCount) noexcept;
      void
      toClap(clap_audio_buffer *buffer, uint32_t frameOffset, uint32_t frameCount) const noexcept;

      // Store op(a, b) into this buffer
      template <typename Operator>
      void compute(const Operator &op,
                   const AudioBuffer<T> &a,
                   const AudioBuffer<T> &b,
                   uint32_t numFrames) noexcept;

      void sum(const AudioBuffer<T> &a, const AudioBuffer<T> &b, uint32_t numFrames) noexcept {
         struct Sum {
            T operator()(T a, T b) const noexcept { return a + b; }
         } op;

         compute(op, a, b, numFrames);
      }

      void product(const AudioBuffer<T> &a, const AudioBuffer<T> &b, uint32_t numFrames) noexcept {
         struct Sum {
            T operator()(T a, T b) const noexcept { return a * b; }
         } op;

         compute(op, a, b, numFrames);
      }

      template <typename Operator>
      void applyTo(const Operator &op, uint32_t numFrames) noexcept;

      void copy(const AudioBuffer<T> &a, uint32_t numFrames) noexcept {
         assert(numFrames <= _frameCount);
         assert(numFrames <= a._frameCount);

         if (a.isConstant()) {
            setConstant(true);
            if (_channelCount == a._channelCount) {
               std::copy_n(a._data, _channelCount, _data);
            } else if (a._channelCount == 1) {
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[c] = a._data[0];
            }
         } else {
            setConstant(false);
            if (_channelCount == a._channelCount) {
               std::copy_n(a._data, numFrames * _channelCount, _data);
            } else if (a._channelCount == 1) {
               for (uint32_t i = 0; i < numFrames; ++i)
                  for (uint32_t c = 0; c < _channelCount; ++c)
                     _data[i * _channelCount + c] = a._data[i];
            }
         }
      }

      void clear(const T &value = T{}) {
         for (uint32_t c = 0; c < _channelCount; ++c)
            _data[c] = value;
         setConstant(true);
      }

   private:
      const uint32_t _channelCount;
      const uint32_t _frameCount;
      const double _sampleRate; // 0 if unspecified
      T *const _data;
      uint32_t _stride = 0;
   };

   extern template class AudioBuffer<float>;
   extern template class AudioBuffer<double>;
} // namespace clap

#include "audio-buffer.hxx"