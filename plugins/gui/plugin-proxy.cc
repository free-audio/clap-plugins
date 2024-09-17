#include <variant>

#include "../modules/module.hh"
#include "abstract-gui-listener.hh"
#include "gui.hh"
#include "plugin-proxy.hh"

namespace clap {
   PluginProxy::PluginProxy(Gui &client) : super(&client), _client(client) {}

   ParameterProxy *PluginProxy::param(clap_id paramId) {
      auto it = _parameters.find(paramId);
      if (it != _parameters.end())
         return it->second;

      auto *p = new ParameterProxy(_client, paramId);
      _parameters.insert_or_assign(paramId, p);
      return p;
   }

   ParameterProxy *PluginProxy::param(clap_id moduleId, clap_id paramId) {
      return param(Module::parameter_capacity * moduleId + paramId);
   }

   QString PluginProxy::toString() const { return "Plugin"; }

   void PluginProxy::defineParameter(const clap_param_info &info) {
      auto it = _parameters.emplace(info.id, new ParameterProxy(_client, info));
      if (!it.second)
         it.first->second->redefine(info);
   }

   void PluginProxy::invoke(const QString &method) {
      _client.guiListener().onGuiInvoke(method.toStdString(), {});
   }

   void PluginProxy::invoke(const QString &method, const QVariantList &args) {
      InvocationArgumentsType targetArgs;
      for (auto &a : args) {
         if (get_if<double>(&a))
            targetArgs.push_back(get<double>(a));
         else if (get_if<int64_t>(&a))
            targetArgs.push_back(get<int64_t>(a));
         else if (get_if<bool>(&a))
            targetArgs.push_back(get<bool>(a));
         else if (get_if<QString>(&a))
            targetArgs.push_back(get<QString>(a).toStdString());
      }
      _client.guiListener().onGuiInvoke(method.toStdString(), targetArgs);
   }

   void PluginProxy::setGuiProperty(const std::string &name,
                                    const AbstractGui::PropertyValue &value) {
      const auto qname = QString::fromStdString(name);
      const auto qvalue = QVariant::fromStdVariant(value);

      _properties.insert(qname, qvalue);
      guiPropertiesChanged();
   }

   const QVariantMap &PluginProxy::guiProperties() const noexcept { return _properties; }
} // namespace clap