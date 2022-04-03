#pragma once

#include <string>

#include "../audio-buffer.hh"

namespace clap {
   class ValueType {
   public:
      virtual ~ValueType();

      [[nodiscard]] virtual std::string toText(double paramValue) const = 0;
      [[nodiscard]] virtual double fromText(const std::string &paramValueText) const = 0;

      /* Domain conversion */
      [[nodiscard]] virtual bool hasEngineDomain() const;
      [[nodiscard]] virtual double toParam(double engineValue) const;
      [[nodiscard]] virtual double toEngine(double paramValue) const;
      virtual void toEngine(AudioBuffer<double> &buffer, uint32_t numFrames) const;
   };
} // namespace clap
