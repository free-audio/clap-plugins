#pragma once

#include <QObject>
#include <QColor>

#include <clap/clap.h>

namespace clap {
   class Gui;
   class TrackInfoProxy : public QObject {
      using super = QObject;

      Q_OBJECT
      Q_PROPERTY(bool hasInfo READ hasInfo NOTIFY hasInfoChanged)
      Q_PROPERTY(QString name READ name NOTIFY nameChanged)
      Q_PROPERTY(QColor color READ color NOTIFY colorChanged)
      Q_PROPERTY(uint64_t flags READ flags NOTIFY flagsChanged)
      Q_PROPERTY(int32_t audioChannelCount READ audioChannelCount NOTIFY audioChannelCountChanged)
      Q_PROPERTY(QString audioPortType READ audioPortType NOTIFY audioPortTypeChanged)
      Q_PROPERTY(bool isForReturn READ isForReturn NOTIFY isForReturnChanged)
      Q_PROPERTY(bool isForBus READ isForBus NOTIFY isForBusChanged)
      Q_PROPERTY(bool isForMaster READ isForMaster NOTIFY isForMasterChanged)

   public:
      explicit TrackInfoProxy(Gui &client);

      void clear();
      void update(bool hasTrackInfo, const clap_track_info &info);

      [[nodiscard]] bool hasInfo() const noexcept { return _hasInfo; }
      [[nodiscard]] bool isForReturn() const noexcept { return _isForReturn; }
      [[nodiscard]] bool isForBus() const noexcept { return _isForBus; }
      [[nodiscard]] bool isForMaster() const noexcept { return _isForMaster; }
      [[nodiscard]] const QString& name() const noexcept { return _name; }
      [[nodiscard]] const QColor& color() const noexcept { return _color; }
      [[nodiscard]] uint64_t flags() const noexcept { return _flags; }
      [[nodiscard]] int32_t audioChannelCount() const noexcept { return _audioChannelCount; }
      [[nodiscard]] const QString& audioPortType() const noexcept { return _audioPortType; }

   signals:
      void updated();

      void hasInfoChanged();
      void isForBusChanged();
      void isForMasterChanged();
      void isForReturnChanged();
      void nameChanged();
      void colorChanged();
      void flagsChanged();
      void audioChannelCountChanged();
      void audioPortTypeChanged();

   private:
      using NotifyType = void (TrackInfoProxy::*)();

      template <typename T>
      void update(T &attr, T value, NotifyType notify) {
         if (value == attr)
            return;
         attr = value;
         (this->*notify)();
      }

      Gui &_client;

      bool _hasInfo = false;
      bool _isForReturn = false;
      bool _isForBus = false;
      bool _isForMaster = false;
      QString _name;
      QColor _color;
      uint64_t _flags;
      int32_t _audioChannelCount;
      QString _audioPortType;
   };
} // namespace clap