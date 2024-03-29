#include <QRgb>

#include "../io/messages.hh"
#include "gui.hh"
#include "track-info-proxy.hh"

namespace clap {
   TrackInfoProxy::TrackInfoProxy(Gui &client) : super(&client), _client(client) {}

   void TrackInfoProxy::clear() { update<bool>(_hasInfo, false, &TrackInfoProxy::hasInfoChanged); }

   void TrackInfoProxy::update(bool hasTrackInfo, const clap_track_info &info) {
      update<bool>(_hasInfo, hasTrackInfo, &TrackInfoProxy::hasInfoChanged);

      const QString name = QString::fromUtf8(info.name, strnlen(info.name, sizeof(info.name)));
      update<QString>(_name, name, &TrackInfoProxy::nameChanged);
      update<uint64_t>(_flags, info.flags, &TrackInfoProxy::flagsChanged);
      update<int32_t>(
         _audioChannelCount, info.audio_channel_count, &TrackInfoProxy::audioChannelCountChanged);
      update<QString>(_audioPortType,
                      QString::fromUtf8(info.audio_port_type),
                      &TrackInfoProxy::audioPortTypeChanged);

      const auto &c = info.color;
      update<QColor>(
         _color, QColor(c.red, c.green, c.blue, c.alpha), &TrackInfoProxy::colorChanged);
      update<bool>(
         _isForBus, info.flags & CLAP_TRACK_INFO_IS_FOR_BUS, &TrackInfoProxy::isForBusChanged);
      update<bool>(_isForReturn,
                   info.flags & CLAP_TRACK_INFO_IS_FOR_RETURN_TRACK,
                   &TrackInfoProxy::isForReturnChanged);
      update<bool>(_isForMaster,
                   info.flags & CLAP_TRACK_INFO_IS_FOR_MASTER,
                   &TrackInfoProxy::isForMasterChanged);

      emit updated();
   }
} // namespace clap