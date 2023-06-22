#include "sample-delay.hh"

namespace clap {
   template <typename T>
   void SampleDelay<T>::setDelayTime(uint32_t delayTimeInFrames) {
      _delayTime = delayTimeInFrames;
      _data.resize(delayTimeInFrames * _channels);
   }

   template <typename T>
   void SampleDelay<T>::reset(const T& value) {
      std::fill(_data.begin(), _data.end(), value);
   }

   template <typename T>
   void SampleDelay<T>::process(const AudioBuffer<T> &in, AudioBuffer<T> &out, uint32_t numFrames) {
      assert(in.channelCount() == _channels);
      assert(out.channelCount() == _channels);
      assert(numFrames <= in.frameCount());
      assert(numFrames <= out.frameCount());

      out.setConstant(false);

      // Naive impl
      for (uint32_t i = 0; i < numFrames; ++i) {
         assert(_pos % _channels == 0);
         assert(_pos < _data.size());

         for (uint32_t c = 0; c < _channels; ++c) {
            out.setSample(i, c, _data[_pos]);
            _data[_pos] = in.getSample(i, c);
            _pos = (_pos + 1) % _data.size();
         }
      }
   }
} // namespace clap

template class clap::SampleDelay<float>;
template class clap::SampleDelay<double>;
