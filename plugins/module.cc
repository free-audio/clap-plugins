#include "module.hh"

namespace clap {

   Module::Module(CorePlugin &plugin, std::string name, clap_id paramIdStart)
      : _plugin(plugin), _name(std::move(name)), _paramIdStart(paramIdStart) {}

   Module::~Module() = default;

} // namespace clap
