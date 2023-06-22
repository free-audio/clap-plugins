#include <vector>

#include "audio-buffer.hh"

namespace clap {
   template <typename T>
   class SampleDelay {
   public:
      SampleDelay(uint32_t channels) : _channels(channels) {}

      void setDelayTime(uint32_t delayTimeInFrames);
      uint32_t getDelayTime() const noexcept { return _delayTime; }
      void reset(const T& value);

      void process(const AudioBuffer<T> &in, AudioBuffer<T> &out, uint32_t numFrames);

   private:

      const uint32_t _channels;
      uint32_t _delayTime = 0; // in frames
      uint32_t _pos = 0;
      std::vector<T> _data;
   };
} // namespace clap

extern template class clap::SampleDelay<float>;
extern template class clap::SampleDelay<double>;