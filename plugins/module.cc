#include "module.hh"
#include "core-plugin.hh"

namespace clap {

   Module::Module(CorePlugin &plugin, std::string name, clap_id paramIdStart)
      : _plugin(plugin), _name(std::move(name)), _paramIdStart(paramIdStart) {}

   Module::~Module() = default;

   Module *Module::cloneVoice() const { return nullptr; }

   Parameter *Module::addParameter(
      uint32_t id, const std::string &name, uint32_t flags, double min, double max, double deflt) {
      clap_param_info info;
      info.id = _paramIdStart + id;
      info.cookie = nullptr;
      info.flags = flags;
      info.min_value = min;
      info.max_value = max;
      info.default_value = deflt;
      snprintf(info.name, sizeof(info.name), "%s", name.c_str());
      snprintf(info.module, sizeof(info.module), "/%s", _name.c_str());
      return _plugin._parameters.addParameter(info);
   }

} // namespace clap
