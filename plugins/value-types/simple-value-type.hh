#pragma once

#include <string>

#include "value-type.hh"

namespace clap {
   class SimpleValueType final : public ValueType {
   public:
      SimpleValueType(double min, double max, double deflt);

      double minValue() const noexcept override { return _minValue; }
      double maxValue() const noexcept override { return _maxValue; }
      double defaultValue() const noexcept override { return _defaultValue; }

      std::string toText(double paramValue) const override;
      bool hasEngineDomain() const override;
      double fromText(const std::string &paramValueText) const override;
      double toEngine(double paramValue) const override;
      double toParam(double engineValue) const override;

   private:
      const double _minValue;
      const double _maxValue;
      const double _defaultValue;
   };
} // namespace clap
