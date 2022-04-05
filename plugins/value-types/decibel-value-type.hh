#pragma once

#include <string>

#include "value-type.hh"

namespace clap {
   class DecibelValueType final : public ValueType {
   public:
      DecibelValueType(double min = -120, double max = +120, double defaultValue = 0);

      double minValue() const noexcept override { return _minValue; }
      double maxValue() const noexcept override { return _maxValue; }
      double defaultValue() const noexcept override { return _defaultValue; }

      std::string toText(double paramValue) const override;
      double fromText(const std::string &paramValueText) const override;

      bool hasEngineDomain() const override;
      double toEngine(double paramValue) const override;
      double toParam(double engineValue) const override;

   private:
      const double _minValue;
      const double _maxValue;
      const double _defaultValue;
   };
} // namespace clap
