#include <cmath>
#include <sstream>

#include "frequency-value-type.hh"

namespace clap {

   FrequencyValueType::FrequencyValueType(double min, double max, double dflt)
      : _minValue(min), _maxValue(max), _defaultValue(dflt),
      _a(std::log(min)), _b(std::log(max) - _a) {}

   std::string FrequencyValueType::toText(double paramValue) const {
      std::ostringstream os;
      os << toEngine(paramValue) << " Hz";
      return os.str();
   }

   double FrequencyValueType::fromText(const std::string &paramValueText) const {
      std::istringstream is(paramValueText);
      double value = 0;
      is >> value;
      return toParam(value);
   }

   double FrequencyValueType::toEngine(double paramValue) const {
      return std::exp(_a + paramValue * _b);
   }

   double FrequencyValueType::toParam(double engineValue) const {
      return (log(engineValue) - _a) / _b;
   }

   bool FrequencyValueType::hasEngineDomain() const { return true; }

} // namespace clap
