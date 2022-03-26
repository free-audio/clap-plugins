#include "adsr-module.hh"

namespace clap {

   AdsrModule::AdsrModule(CorePlugin &plugin, std::string name, clap_id paramIdStart)
      : Module(plugin, name, paramIdStart) {}

   AdsrModule::~AdsrModule() = default;

} // namespace clap
