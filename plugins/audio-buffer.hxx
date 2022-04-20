#pragma once

#include <cassert>

#include "audio-buffer.hh"

namespace clap {
   template <typename T>
   template <typename Operator>
   void AudioBuffer<T>::compute(const Operator &op,
                                const AudioBuffer<T> &a,
                                const AudioBuffer<T> &b,
                                uint32_t numFrames) noexcept {
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

      if (a.isConstant()) {
         if (a.channelCount() == 1 && b.channelCount() == 1) {
            T va = a._data[0];
            for (uint32_t i = 0; i < numFrames; ++i) {
               T v = op(va, b._data[i * b._stride]);
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = v;
            }
         } else if (a.channelCount() == 1) {
            T va = a._data[0];
            for (uint32_t i = 0; i < numFrames; ++i) {
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = op(va, b._data[i * b._stride + c]);
            }
         } else if (b.channelCount() == 1) {
            T va[BLOCK_SIZE];
            for (uint32_t c = 0; c < a._channelCount; ++c)
               va[c] = a._data[c];

            for (uint32_t i = 0; i < numFrames; ++i) {
               T vb = b._data[i * b._stride];
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = op(va[c], vb);
            }
         } else {
            T va[BLOCK_SIZE];
            for (uint32_t c = 0; c < a._channelCount; ++c)
               va[c] = a._data[c];

            for (uint32_t i = 0; i < numFrames; ++i) {
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = op(va[c], b._data[i * b._stride + c]);
            }
         }
         setConstant(false);
         return;
      }

      if (b.isConstant()) {
         if (a.channelCount() == 1 && b.channelCount() == 1) {
            T vb = b._data[0];
            for (uint32_t i = 0; i < numFrames; ++i) {
               T v = op(a._data[i * a._stride], vb);
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = v;
            }
         } else if (a.channelCount() == 1) {
            T vb[BLOCK_SIZE];
            for (uint32_t c = 0; c < a._channelCount; ++c)
               vb[c] = b._data[c];

            for (uint32_t i = 0; i < numFrames; ++i) {
               T va = a._data[i * a._stride];
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = op(va, vb[c]);
            }
         } else if (b.channelCount() == 1) {
            T vb = b._data[0];
            for (uint32_t i = 0; i < numFrames; ++i) {
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = op(a._data[i * a._stride + c], vb);
            }
         } else {
            T vb[BLOCK_SIZE];
            for (uint32_t c = 0; c < a._channelCount; ++c)
               vb[c] = b._data[c];

            for (uint32_t i = 0; i < numFrames; ++i) {
               for (uint32_t c = 0; c < _channelCount; ++c)
                  _data[i * _channelCount + c] = op(a._data[i * a._stride + c], vb[c]);
            }
         }
         setConstant(false);
         return;
      }

      if (a.channelCount() == 1 && b.channelCount() == 1) {
         for (uint32_t i = 0; i < numFrames; ++i) {
            T v = op(a._data[i * a._stride], b._data[i * b._stride]);
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[i * _channelCount + c] = v;
         }
      } else if (a.channelCount() == 1) {
         for (uint32_t i = 0; i < numFrames; ++i) {
            T va = a._data[i * a._stride];
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[i * _channelCount + c] = op(va, b._data[i * b._stride + c]);
         }
      } else if (b.channelCount() == 1) {
         for (uint32_t i = 0; i < numFrames; ++i) {
            T vb = b._data[i * b._stride];
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[i * _channelCount + c] = op(a._data[i * a._stride + c], vb);
         }
      } else {
         for (uint32_t i = 0; i < numFrames; ++i) {
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[i * _channelCount + c] =
                  op(a._data[i * a._stride + c], b._data[i * b._stride + c]);
         }
      }
      setConstant(false);
   }

   template <typename T>
   template <typename Operator>
   void AudioBuffer<T>::compute(const Operator &op, uint32_t numFrames) noexcept {
      assert(numFrames <= _frameCount);
      const uint32_t N = _channelCount * (isConstant() ? 1 : numFrames);
      for (uint32_t i = 0; i < N; ++i)
         _data[i] = op(_data[i]);
   }
} // namespace clap
