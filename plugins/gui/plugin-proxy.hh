#pragma once

#include <unordered_map>

#include <QObject>

#include <clap/clap.h>

#include "parameter-proxy.hh"

namespace clap {

   class Gui;
   class PluginProxy : public QObject {
      using super = QObject;

      Q_OBJECT

   public:
      explicit PluginProxy(Gui &client);

      void defineParameter(const clap_param_info &info);

      Q_INVOKABLE ParameterProxy *param(clap_id paramId);
      Q_INVOKABLE ParameterProxy *param(clap_id moduleId, clap_id paramId);
      Q_INVOKABLE QString toString() const;

   private:
      Gui &_client;
      std::unordered_map<clap_id, ParameterProxy *> _parameters;
   };

} // namespace clap