#pragma once

#include "audio-buffer.hh"

namespace clap {
   template <typename T>
   template <typename Operator>
   void AudioBuffer<T>::compute(const Operator& op, AudioBuffer<T> &a, AudioBuffer<T> &b, uint32_t numFrames) noexcept {
      assert(numFrames <= _frameCount);
      assert(a.channelCount() == 1 || a.channelCount() == _channelCount);
      assert(b.channelCount() == 1 || b.channelCount() == _channelCount);

      /* both are constant, fast path */
      if (a.isConstant() && b.isConstant()) {
         if (a.channelCount() == 1 && b.channelCount() == 1) {
            T v = op(a._data[0], b._data[0]);
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[c] = v;
         } else if (a.channelCount() == 1) {
            T va = a._data[0];
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[c] = op(va, b._data[c]);
         } else if (b.channelCount() == 1) {
            T vb = b._data[0];
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[c] = op(a._data[c], vb);
         } else {
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[c] = op(a._data[c], b._data[c]);
         }
         setConstant(true);
         return;
      }

      if (a.channelCount() == 1 && b.channelCount() == 1) {
         for (uint32_t i = 0; i < numFrames; ++i) {
            T v = op(a._data[i], b._data[i]);
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[i * _channelCount + c] = v;
         }
      } else if (a.channelCount() == 1) {
         for (uint32_t i = 0; i < numFrames; ++i) {
            T va = a._data[i];
            for (uint32_t c = 0; c < _channelCount; ++c) {
               auto index = i * _channelCount + c;
               _data[index] = op(va, b._data[index]);
            }
         }
      } else if (b.channelCount() == 1) {
         for (uint32_t i = 0; i < numFrames; ++i) {
            T vb = b._data[i];
            for (uint32_t c = 0; c < _channelCount; ++c) {
               auto index = i * _channelCount + c;
               _data[index] = op(a._data[index], vb);
            }
         }
      } else {
         for (uint32_t i = 0; i < numFrames; ++i) {
            for (uint32_t c = 0; c < _channelCount; ++c) {
               auto index = i * _channelCount + c;
               _data[index] = op(a._data[index], b._data[index]);
            }
         }
      }
      setConstant(false);
   }
} // namespace clap