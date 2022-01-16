#pragma once

#include <unordered_map>

#include <QObject>

#include <clap/clap.h>

#include "parameter-proxy.hh"

class GuiClient;
class PluginProxy : public QObject {
   using super = QObject;

   Q_OBJECT

public:
   explicit PluginProxy(GuiClient& client);

   void defineParameter(const clap_param_info& info);

   Q_INVOKABLE ParameterProxy *param(clap_id paramId);
   Q_INVOKABLE QString toString() const;

private:
   GuiClient& _client;
   std::unordered_map<clap_id, ParameterProxy *> _parameters;
};