#include <sstream>

#include "simple-value-type.hh"

namespace clap {

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

   double SimpleValueType::toEngine(double paramValue) const { return paramValue; }
   double SimpleValueType::toParam(double engineValue) const { return engineValue; }
} // namespace clap