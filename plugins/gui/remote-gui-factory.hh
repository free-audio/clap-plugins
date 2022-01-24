#pragma once

#include <unordered_map>

#include <QObject>

#include "../io/messages.hh"
#include "../io/remote-channel.hh"

#include "gui-client.hh"

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE

namespace clap {
   class RemoteGuiFactory : public QObject, public BasicRemoteChannel::EventControl {
   public:
      RemoteGuiFactory(int socket);

      RemoteGuiFactory(void *pipeIn, void *pipeOut);

      GuiClient *getClient(uint32_t clientId) const;

   private:
      template <class T>
      GuiClient *getClient(const T &msg) const {
         return getClient(msg.clientId);
      }

      void onMessage(const clap::RemoteChannel::Message &msg);

      void modifyFd(int flags) override;
      void removeFd() override;

      friend class RemoteGuiListener;

      std::unordered_map<uint32_t, std::unique_ptr<GuiClient>> _guiClients;

      std::unique_ptr<clap::RemoteChannel> _channel;
      std::unique_ptr<QSocketNotifier> _socketReadNotifier;
      std::unique_ptr<QSocketNotifier> _socketWriteNotifier;
   };

} // namespace clap