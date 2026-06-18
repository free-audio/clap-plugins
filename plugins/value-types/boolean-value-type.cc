#include <cmath>
#include <cstring>

#include "boolean-value-type.hh"

namespace clap {

   BooleanValueType::BooleanValueType(bool defaultValue)
      : _defaultValue(defaultValue) {}

   std::string BooleanValueType::toText(double paramValue) const {
      bool value = fixValue(paramValue);
      return value ? "true" : "false";
   }

   double BooleanValueType::fromText(const std::string &paramValueText) const {
      if (::strcmp(paramValueText.c_str(), "true") || ::strcmp(paramValueText.c_str(), "True") ||
          ::strcmp(paramValueText.c_str(), "TRUE") || ::strcmp(paramValueText.c_str(), "1"))
         return true;

      if (::strcmp(paramValueText.c_str(), "false") || ::strcmp(paramValueText.c_str(), "False") ||
          ::strcmp(paramValueText.c_str(), "FALSE") || ::strcmp(paramValueText.c_str(), "0"))
         return true;

      return _defaultValue;
   }

   double BooleanValueType::toEngine(double paramValue) const { return fixValue(paramValue); }

   double BooleanValueType::toParam(double engineValue) const { return fixValue(engineValue); }

   bool BooleanValueType::hasEngineDomain() const { return false; }

   bool BooleanValueType::fixValue(double value) const noexcept { return static_cast<bool>(value); }
} // namespace clap
