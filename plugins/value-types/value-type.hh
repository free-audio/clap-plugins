#pragma once

#include <string>

namespace clap {
   class ValueType {
   public:
      virtual ~ValueType();

      virtual std::string toText(double paramValue) const = 0;
      virtual double fromText(const std::string &paramValueText) const = 0;
      virtual double toEngine(double paramValue) const = 0;
      virtual double toParam(double engineValue) const = 0;
   };
} // namespace clap
