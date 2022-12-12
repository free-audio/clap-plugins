#include <cassert>

#include "audio-buffer.hh"

namespace clap {

   template <typename T>
   void AudioBuffer<T>::fromClap(const clap_audio_buffer *buffer,
                                 uint32_t frameOffset,
                                 uint32_t frameCount) noexcept {
      assert(frameCount <= _frameCount);
      assert(_channelCount == buffer->channel_count);

      const uint32_t allConstantMask = (1 << _channelCount) - 1;
      const bool allConstant = (buffer->constant_mask & allConstantMask) == allConstantMask;

      if (allConstant) {
         if (buffer->data32) {
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[c] = static_cast<T>(buffer->data32[c][0]);
         } else {
            for (uint32_t c = 0; c < _channelCount; ++c)
               _data[c] = static_cast<T>(buffer->data64[c][0]);
         }
         _stride = 0;
         return;
      }

      if (buffer->data32) {
         for (uint32_t c = 0; c < _channelCount; ++c) {
            const uint32_t stride = (buffer->constant_mask & (1 << c)) ? 0 : 1;
            for (uint32_t i = 0; i < frameCount; ++i) {
               const uint32_t index = stride * (i + frameOffset);
               _data[i * _channelCount + c] = static_cast<T>(buffer->data32[c][index]);
            }
         }
      } else {
         for (uint32_t c = 0; c < _channelCount; ++c) {
            const uint32_t stride = (buffer->constant_mask & (1 << c)) ? 0 : 1;
            for (uint32_t i = 0; i < frameCount; ++i) {
               const uint32_t index = stride * (i + frameOffset);
               _data[i * _channelCount + c] = static_cast<T>(buffer->data64[c][index]);
            }
         }
      }
      _stride = _channelCount;
   }

   template <typename T>
   void AudioBuffer<T>::toClap(clap_audio_buffer *buffer,
                               uint32_t frameOffset,
                               uint32_t frameCount) const noexcept {
      assert(frameCount <= _frameCount);
      assert(_channelCount == buffer->channel_count);

#if 0
      const uint32_t allConstantMask = (1 << _channelCount) - 1;

      // This is the first block and it is constant
      if (_stride == 0 && frameOffset == 0) {
         if (buffer->data32) {
            for (uint32_t c = 0; c < _channelCount; ++c)
               buffer->data32[c][0] = static_cast<float>(_data[c]);
         } else {
            for (uint32_t c = 0; c < _channelCount; ++c)
               buffer->data64[c][0] = static_cast<double>(_data[c]);
         }
         buffer->constant_mask = allConstantMask;
         return;
      }

      // This isn't the first block and it is constant, and the block before were constant too
      const bool allConstant = ((buffer->constant_mask & allConstantMask) == allConstantMask);
      if (_stride == 0 && frameOffset != 0 && allConstant) {
         // check if the constant value is the same as in the previous block
         bool same = true;
         if (buffer->data32) {
            for (uint32_t c = 0; same && c < _channelCount; ++c)
               same = (_data[c] == buffer->data32[c][0]);
         } else {
            for (uint32_t c = 0; c < _channelCount; ++c)
               same = (_data[c] == buffer->data32[c][0]);
         }

         if (same)
            return;
      }

      // The previous block were constant but this one isn't, we need to fill the buffer
      if (allConstant && _stride > 0) {
         if (buffer->data32) {
            for (uint32_t c = 0; c < _channelCount; ++c)
               for (uint32_t i = 0; i < frameOffset; ++i)
                  buffer->data32[c][i] = buffer->data32[c][0];
         } else {
            for (uint32_t c = 0; c < _channelCount; ++c)
               for (uint32_t i = 0; i < frameOffset; ++i)
                  buffer->data64[c][i] = buffer->data64[c][0];
         }
      }
#endif

      // Copy this block
      if (buffer->data32) {
         for (uint32_t c = 0; c < _channelCount; ++c)
            for (uint32_t i = 0; i < frameCount; ++i) {
               buffer->data32[c][i + frameOffset] = static_cast<float>(_data[i * _stride + c]);
            }
      } else {
         for (uint32_t c = 0; c < _channelCount; ++c)
            for (uint32_t i = 0; i < frameCount; ++i) {
               buffer->data64[c][i + frameOffset] = static_cast<double>(_data[i * _stride + c]);
            }
      }
      buffer->constant_mask = 0;
   }

   template <typename T>
   void AudioBuffer<T>::fromClap(const clap_audio_buffer *buffer,
                                 uint32_t frameOffset,
                                 uint32_t frameCount,
                                 uint32_t src_channel,
                                 uint32_t dst_channel) noexcept {
      assert(frameCount <= _frameCount);
      assert(src_channel < buffer->channel_count);
      assert(dst_channel < _channelCount);

      const uint32_t constantMask = 1 << src_channel;
      const bool isConstant = buffer->constant_mask & constantMask;
      const uint32_t stride = isConstant ? 0 : 1;

      if (buffer->data32) {
         for (uint32_t i = 0; i < frameCount; ++i) {
            const uint32_t index = stride * (i + frameOffset);
            _data[i * _channelCount + dst_channel] =
               static_cast<T>(buffer->data32[src_channel][index]);
         }
      } else {
         for (uint32_t i = 0; i < frameCount; ++i) {
            const uint32_t index = stride * (i + frameOffset);
            _data[i * _channelCount + dst_channel] =
               static_cast<T>(buffer->data64[src_channel][index]);
         }
      }
      _stride = _channelCount;
   }

   template <typename T>
   void AudioBuffer<T>::toClap(clap_audio_buffer *buffer,
                               uint32_t frameOffset,
                               uint32_t frameCount,
                               uint32_t src_channel,
                               uint32_t dst_channel) const noexcept {
      assert(frameCount <= _frameCount);
      assert(src_channel < _channelCount);
      assert(dst_channel < buffer->channel_count);

      // Copy this block
      if (buffer->data32) {
         for (uint32_t i = 0; i < frameCount; ++i)
            buffer->data32[dst_channel][i + frameOffset] =
               static_cast<float>(_data[i * _stride + src_channel]);
      } else {
         for (uint32_t i = 0; i < frameCount; ++i)
            buffer->data64[dst_channel][i + frameOffset] =
               static_cast<double>(_data[i * _stride + src_channel]);
      }
      buffer->constant_mask = 0;
   }

   template class AudioBuffer<float>;
   template class AudioBuffer<double>;
} // namespace clap
