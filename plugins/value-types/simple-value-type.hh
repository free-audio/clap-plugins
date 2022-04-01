#pragma once

#include <string>

#include "value-type.hh"

namespace clap {
   class SimpleValueType final : public ValueType {
      std::string toText(double paramValue) const override;
      double fromText(const std::string &paramValueText) const override;
      double toEngine(double paramValue) const override;
      double toParam(double engineValue) const override;
   };
} // namespace clap
