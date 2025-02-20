#pragma once

#include <string>
#include <vector>

#include "value-type.hh"

namespace clap {
   class BooleanValueType final : public ValueType {
   public:
      BooleanValueType(std::vector<std::string> entries, bool defaultValue);

      bool isStepped() const noexcept override { return true; }

      double minValue() const noexcept override { return false; }
      double maxValue() const noexcept override { return true; }
      double defaultValue() const noexcept override { return _defaultValue; }

      std::string toText(double paramValue) const override;
      bool hasEngineDomain() const override;
      double fromText(const std::string &paramValueText) const override;
      double toEngine(double paramValue) const override;
      double toParam(double engineValue) const override;

      bool fixValue(double value) const noexcept;

   private:
      const bool _defaultValue;
   };
} // namespace clap
