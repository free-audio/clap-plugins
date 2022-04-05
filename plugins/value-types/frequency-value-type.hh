#pragma once

#include <string>

#include "value-type.hh"

namespace clap {
   class FrequencyValueType final : public ValueType {
   public:
      FrequencyValueType(double min = 20, double max = 20000, double defaultValue = 345);

      double minValue() const noexcept override { return 0; }
      double maxValue() const noexcept override { return 1; }
      double defaultValue() const noexcept override { return toParam(_defaultValue); }

      std::string toText(double paramValue) const override;
      double fromText(const std::string &paramValueText) const override;

      bool hasEngineDomain() const override;
      double toEngine(double paramValue) const override;
      double toParam(double engineValue) const override;

   private:
      const double _minValue;
      const double _maxValue;
      const double _defaultValue;

      const double _a;
      const double _b;
   };
} // namespace clap
