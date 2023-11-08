#include <QRgb>

#include "../io/messages.hh"
#include "abstract-gui-listener.hh"
#include "gui.hh"
#include "track-info-proxy.hh"

namespace clap {
   TrackInfoProxy::TrackInfoProxy(Gui &client) : super(&client), _client(client) {}

   void TrackInfoProxy::clear() { update<bool>(_hasInfo, false, &TrackInfoProxy::hasInfoChanged); }

   void TrackInfoProxy::update(bool hasTrackInfo, const clap_track_info &info) {
      update<bool>(_hasInfo, hasTrackInfo, &TrackInfoProxy::hasInfoChanged);
      update<QString>(
         _name, QString::fromUtf8(info.name, sizeof(info.name)), &TrackInfoProxy::nameChanged);
      update<uint64_t>(_flags, info.flags, &TrackInfoProxy::flagsChanged);
      update<int32_t>(
         _audioChannelCount, info.audio_channel_count, &TrackInfoProxy::audioChannelCountChanged);
      update<QString>(_audioPortType,
                      QString::fromUtf8(info.audio_port_type),
                      &TrackInfoProxy::audioPortTypeChanged);

      const auto &c = info.color;
      update<QColor>(
         _color, QColor(c.alpha, c.red, c.green, c.blue), &TrackInfoProxy::colorChanged);

      emit updated();
   }
} // namespace clap