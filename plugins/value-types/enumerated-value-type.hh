#pragma once

#include <string>

#include "value-type.hh"

namespace clap {
   class EnumeratedValueType final : public ValueType {
   public:
      EnumeratedValueType(std::vector<std::string> entries, uint32_t defaultIndex);

      bool isStepped() const noexcept override { return true; }

      double minValue() const noexcept override { return 0; }
      double maxValue() const noexcept override { return _entries.size() - 1; }
      double defaultValue() const noexcept override { return _defaultIndex; }

      std::string toText(double paramValue) const override;
      bool hasEngineDomain() const override;
      double fromText(const std::string &paramValueText) const override;
      double toEngine(double paramValue) const override;
      double toParam(double engineValue) const override;

      int fixValue(double value) const noexcept;

   private:
      const std::vector<std::string> _entries;
      const uint32_t _defaultIndex;
   };
} // namespace clap
