#include <sstream>
#include <algorithm>

#include "simple-value-type.hh"

namespace clap {

   SimpleValueType::SimpleValueType(double min, double max, double deflt)
      : _minValue(min), _maxValue(max), _defaultValue(deflt) {}

   std::string SimpleValueType::toText(double paramValue) const {
      std::ostringstream os;
      os << paramValue;
      return os.str();
   }

   double SimpleValueType::fromText(const std::string &paramValueText) const {
      std::istringstream is(paramValueText);
      double value = 0;
      is >> value;
      return value;
   }

   double SimpleValueType::toEngine(double paramValue) const { return std::clamp<double>(paramValue, _minValue, _maxValue); }

   double SimpleValueType::toParam(double engineValue) const { return std::clamp<double>(engineValue, _minValue, _maxValue); }

   bool SimpleValueType::hasEngineDomain() const { return false; }

} // namespace clap
