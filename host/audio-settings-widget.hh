#pragma once

#include <QWidget>

class AudioSettings;

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class AudioSettingsWidget : public QWidget {
   Q_OBJECT
public:
   explicit AudioSettingsWidget(AudioSettings &audioSettings);

signals:

public slots:
   void selectedDeviceChanged(int index);
   void selectedSampleRateChanged(int index);
   void selectedBufferSizeChanged(int index);

private:
   AudioSettings &_audioSettings;
   QComboBox *    _sampleRateWidget;
   QComboBox *    _bufferSizeWidget;
};
