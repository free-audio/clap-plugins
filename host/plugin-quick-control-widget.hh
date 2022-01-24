#pragma once

#include <QWidget>

#include <clap/clap.h>

QT_BEGIN_NAMESPACE
class QDial;
class QLabel;
QT_END_NAMESPACE

class PluginHost;
class PluginParam;
class PluginQuickControlWidget : public QWidget {
   Q_OBJECT;

public:
   PluginQuickControlWidget(QWidget *parent, PluginHost &pluginHost);

   void setPluginParam(PluginParam *param);

private:
   void paramInfoChanged();
   void paramValueChanged();
   void dialValueChanged(int newValue);

   void disconnectFromParam();
   void connectToParam(PluginParam *param);

   void updateParamValue();
   void updateParamInfo();
   void updateAll();

   static const constexpr int DIAL_RANGE = 10000;

   PluginHost &pluginHost_;

   QDial *_dial = nullptr;
   QLabel *_label = nullptr;
   PluginParam *_param = nullptr;
};