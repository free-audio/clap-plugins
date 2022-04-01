#include "audio-buffer.hh"

namespace clap {
   template <typename T>
   void AudioBuffer<T>::fromClap(const clap_audio_buffer *buffer,
                                 uint32_t frameOffset,
                                 uint32_t frameCount) noexcept {
      const uint32_t allChannelsConstantMask = (1 << _channelCount) - 1;
      const bool allConstant =
         (buffer->constant_mask & allChannelsConstantMask) == allChannelsConstantMask;

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
         for (uint32_t c = 0; c < _channelCount; ++c)
            for (uint32_t i = 0; i < _frameCount; ++i)
               _data[i * _channelCount + c] = static_cast<T>(buffer->data32[c][i]);
      } else {
         for (uint32_t c = 0; c < _channelCount; ++c)
            for (uint32_t i = 0; i < _frameCount; ++i)
               _data[i * _channelCount + c] = static_cast<T>(buffer->data64[c][i]);
      }
      _stride = _channelCount;
   }

   template <typename T>
   void AudioBuffer<T>::toClap(clap_audio_buffer *buffer,
                               uint32_t frameOffset,
                               uint32_t frameCount) noexcept {}

   template class AudioBuffer<float>;
   template class AudioBuffer<double>;
} // namespace clap