#include "module.hh"
#include "../core-plugin.hh"

namespace clap {

   Module::Module(CorePlugin &plugin, std::string name, clap_id paramIdStart)
      : _plugin(plugin), _name(std::move(name)), _paramIdStart(paramIdStart) {}

   Module::~Module() = default;

   std::unique_ptr<Module> Module::cloneVoice() const { return {}; }

   Parameter *Module::addParameter(
      uint32_t id, const std::string &name, uint32_t flags, double min, double max, double deflt, const ValueType& valueType) {
      clap_param_info info;
      info.id = _paramIdStart + id;
      info.cookie = nullptr;
      info.flags = flags;
      info.min_value = min;
      info.max_value = max;
      info.default_value = deflt;
      snprintf(info.name, sizeof(info.name), "%s", name.c_str());
      snprintf(info.module, sizeof(info.module), "/%s", _name.c_str());
      return _plugin._parameters.addParameter(info, valueType);
   }

   bool Module::activate(double sampleRate, uint32_t maxFrameCount) {
      assert(!_isActive);
      _isActive = doActivate(sampleRate, maxFrameCount);
      return _isActive;
   }

   void Module::deactivate() {
      if (!_isActive)
         return;
      doDeactivate();
      _isActive = false;
   }

   bool Module::doActivate(double sampleRate, uint32_t maxFrameCount) { return true; }

   void Module::doDeactivate() {}
} // namespace clap
