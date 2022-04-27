#pragma once

#include <string>
#include <numeric>

#include "../audio-buffer.hh"

namespace clap {
   class ValueType {
   public:
      virtual ~ValueType();

      /* The sample rate may be required to do the conversion to engine */
      virtual void setSampleRate(double sampleRate) noexcept {}

      [[nodiscard]] virtual bool isStepped() const noexcept { return false; }

      [[nodiscard]] virtual double defaultValue() const noexcept { return std::numeric_limits<double>::lowest(); }
      [[nodiscard]] virtual double minValue() const noexcept { return std::numeric_limits<double>::lowest(); }
      [[nodiscard]] virtual double maxValue() const noexcept { return std::numeric_limits<double>::max(); }

      [[nodiscard]] virtual std::string toText(double paramValue) const = 0;
      [[nodiscard]] virtual double fromText(const std::string &paramValueText) const = 0;

      /* Domain conversion */
      [[nodiscard]] virtual bool hasEngineDomain() const;
      [[nodiscard]] virtual double toParam(double engineValue) const;
      [[nodiscard]] virtual double toEngine(double paramValue) const;

      void toEngine(AudioBuffer<double> &buffer, uint32_t numFrames) const;
   };
} // namespace clap
