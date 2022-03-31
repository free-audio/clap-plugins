#include "parameter.hh"

namespace clap {
   Parameter::Parameter(const clap_param_info &info) : _info(info) { _info.cookie = this; }
} // namespace clap
