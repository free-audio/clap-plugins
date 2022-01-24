#pragma once

#include <QWidget>

class Settings;
class AudioSettingsWidget;
class MidiSettingsWidget;

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

class SettingsWidget : public QWidget {
   Q_OBJECT
public:
   explicit SettingsWidget(Settings &settings);

private:
   QTabWidget *_tabWidget = nullptr;
   AudioSettingsWidget *_audioSettingsWidget = nullptr;
   MidiSettingsWidget *_midiSettingsWidget = nullptr;
   Settings &_settings;
};
