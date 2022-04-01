#include "parameter.hh"

namespace clap {
   Parameter::Parameter(const clap_param_info &info, const ValueType &valueType)
      : _info(info), _valueType(valueType) {
      _info.cookie = this;
   }
} // namespace clap
