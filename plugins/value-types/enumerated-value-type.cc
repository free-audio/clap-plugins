#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

#include "enumerated-value-type.hh"

namespace clap {

   EnumeratedValueType::EnumeratedValueType(std::vector<std::string> entries, uint32_t defaultIndex)
      : _entries(std::move(entries)), _defaultIndex(fixValue(defaultIndex)) {}

   std::string EnumeratedValueType::toText(double paramValue) const {
      int index = fixValue(paramValue);
      std::ostringstream os;
      os << _entries[index];
      return os.str();
   }

   double EnumeratedValueType::fromText(const std::string &paramValueText) const {
      for (uint32_t i = 0; i < _entries.size(); ++i) {
         auto &e = _entries[i];
         if (e == paramValueText)
            return i;
      }

      return _defaultIndex;
   }

   double EnumeratedValueType::toEngine(double paramValue) const { return fixValue(paramValue); }

   double EnumeratedValueType::toParam(double engineValue) const { return fixValue(engineValue); }

   bool EnumeratedValueType::hasEngineDomain() const { return false; }

   int EnumeratedValueType::fixValue(double value) const noexcept {
      return std::clamp<int>(value, minValue(), maxValue());
   }
} // namespace clap
