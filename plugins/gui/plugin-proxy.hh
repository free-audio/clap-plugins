#pragma once

#include <unordered_map>

#include <QObject>
#include <QVariantMap>

#include <clap/clap.h>

#include "abstract-gui.hh"
#include "parameter-proxy.hh"

namespace clap {

   class Gui;
   class PluginProxy : public QObject {
      using super = QObject;

      Q_OBJECT

      Q_PROPERTY(QVariantMap props READ guiProperties NOTIFY guiPropertiesChanged)

   signals:
      void guiPropertiesChanged();

   public:
      explicit PluginProxy(Gui &client);

      void setGuiProperty(const std::string &name, const AbstractGui::PropertyValue &value);
      const QVariantMap &guiProperties() const noexcept;

      void defineParameter(const clap_param_info &info);

      Q_INVOKABLE ParameterProxy *param(clap_id paramId);
      Q_INVOKABLE ParameterProxy *param(clap_id moduleId, clap_id paramId);
      Q_INVOKABLE QString toString() const;
      Q_INVOKABLE void invoke(const QString &method);
      Q_INVOKABLE void invoke(const QString &method, const QVariantList &args);

   private:
      Gui &_client;
      std::unordered_map<clap_id, ParameterProxy *> _parameters;

      QVariantMap _properties;
   };

} // namespace clap