#include <cmath>
#include <sstream>

#include "decibel-value-type.hh"

namespace clap {

   DecibelValueType::DecibelValueType(double min, double max, double dflt) : _minValue(min), _maxValue(max), _defaultValue(dflt) {}

   std::string DecibelValueType::toText(double paramValue) const {
      std::ostringstream os;
      os << paramValue << " dB";
      return os.str();
   }

   double DecibelValueType::fromText(const std::string &paramValueText) const {
      std::istringstream is(paramValueText);
      double value = 0;
      is >> value;
      return value;
   }

   double DecibelValueType::toEngine(double paramValue) const {
      return std::pow(10.0, paramValue / 20.0);
   }

   double DecibelValueType::toParam(double engineValue) const {
      return std::log10(20.0 * engineValue);
   }
   bool DecibelValueType::hasEngineDomain() const { return true; }
} // namespace clap
