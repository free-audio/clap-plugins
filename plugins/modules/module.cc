#include <iostream>

#include "../core-plugin.hh"
#include "module.hh"

namespace clap {

   Module::Module(CorePlugin &plugin, std::string name, uint32_t moduleId)
      : _plugin(plugin), _name(std::move(name)), _moduleId(moduleId),
        _paramIdStart(moduleId * parameter_capacity) {}

   Module::~Module() = default;

   std::unique_ptr<Module> Module::cloneVoice() const {
      std::cerr << "missing Module::cloneVoice() implementation" << std::endl;
      std::terminate();
   }

   Parameter *Module::addParameter(uint32_t id,
                                   const std::string &name,
                                   uint32_t flags,
                                   std::unique_ptr<ValueType> valueType) {
      clap_param_info info;
      info.id = _paramIdStart + id;
      info.cookie = nullptr;
      info.flags = flags;
      info.min_value = valueType->minValue();
      info.max_value = valueType->maxValue();
      info.default_value = valueType->defaultValue();
      snprintf(info.name, sizeof(info.name), "%s", name.c_str());
      snprintf(info.module, sizeof(info.module), "/%s", _name.c_str());
      return _plugin.addParameter(info, std::move(valueType));
   }

   bool Module::activate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) {
      assert(!_isActive);
      _isActive = doActivate(sampleRate, maxFrameCount, isRealTime);
      return _isActive;
   }

   void Module::deactivate() {
      if (!_isActive)
         return;
      doDeactivate();
      _isActive = false;
   }

   bool Module::doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) { return true; }

   void Module::doDeactivate() {}
} // namespace clap
