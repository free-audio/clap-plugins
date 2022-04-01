#pragma once

#include <string>

namespace clap {
   class ValueType {
   public:
      virtual ~ValueType() {}

      virtual std::string toText(double paramValue) const;
      virtual double fromText(const std::string &paramValueText) const;
      virtual double toEngine(double paramValue) const;
      virtual double toParam(double engineValue) const;
   };
} // namespace clap
