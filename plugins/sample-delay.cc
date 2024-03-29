#include "sample-delay.hh"

namespace clap {
   template <typename T>
   void SampleDelay<T>::setDelayTime(uint32_t delayTimeInFrames) {
      _delayTime = delayTimeInFrames;
      _data.resize(delayTimeInFrames * _channels);
   }

   template <typename T>
   void SampleDelay<T>::reset(const T &value) {
      std::fill(_data.begin(), _data.end(), value);
   }

   template <typename T>
   void SampleDelay<T>::process(const AudioBuffer<T> &in, AudioBuffer<T> &out, uint32_t numFrames) {
      assert(in.channelCount() == _channels);
      assert(out.channelCount() == _channels);
      assert(numFrames <= in.frameCount());
      assert(numFrames <= out.frameCount());

      if (_delayTime == 0) {
         out.copy(in, numFrames);
         return;
      }

      out.setConstant(false);

      // Naive impl
      const auto dataSz = _data.size();
      for (uint32_t i = 0; i < numFrames; ++i) {
         assert(_pos % _channels == 0);
         assert(_pos < _data.size());

         for (uint32_t c = 0; c < _channels; ++c) {
            const auto rpos = (_pos + dataSz - _delayTime * _channels) % dataSz;
            out.setSample(i, c, _data[rpos]);
            _data[_pos] = in.getSample(i, c);
            _pos = (_pos + 1) % dataSz;
         }
      }
   }
} // namespace clap

template class clap::SampleDelay<float>;
template class clap::SampleDelay<double>;
