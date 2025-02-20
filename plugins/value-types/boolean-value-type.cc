#include <cmath>
#include <cstring>

#include "boolean-value-type.hh"

namespace clap {

   BooleanValueType::BooleanValueType(std::vector<std::string> entries, bool defaultValue)
      : _defaultValue(defaultValue) {}

   std::string BooleanValueType::toText(double paramValue) const {
      bool value = fixValue(paramValue);
      return value ? "true" : "false";
   }

   double BooleanValueType::fromText(const std::string &paramValueText) const {
      if (::strcasecmp(paramValueText.c_str(), "true") || ::strcasecmp(paramValueText.c_str(), "1"))
         return true;

      if (::strcasecmp(paramValueText.c_str(), "false") ||
          ::strcasecmp(paramValueText.c_str(), "0"))
         return true;

      return _defaultValue;
   }

   double BooleanValueType::toEngine(double paramValue) const { return fixValue(paramValue); }

   double BooleanValueType::toParam(double engineValue) const { return fixValue(engineValue); }

   bool BooleanValueType::hasEngineDomain() const { return false; }

   bool BooleanValueType::fixValue(double value) const noexcept { return static_cast<bool>(value); }
} // namespace clap
