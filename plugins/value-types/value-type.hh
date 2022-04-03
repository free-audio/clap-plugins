#pragma once

#include <string>

#include "../audio-buffer.hh"

namespace clap {
   class ValueType {
   public:
      virtual ~ValueType();

      virtual std::string toText(double paramValue) const = 0;
      virtual double fromText(const std::string &paramValueText) const = 0;

      /* Domain conversion */
      virtual double toParam(double engineValue) const = 0;
      virtual double toEngine(double paramValue) const = 0;
      virtual void toEngine(AudioBuffer<double> &buffer, uint32_t numFrames) const;
   };
} // namespace clap
