#pragma once

#include <memory>
#include <unordered_map>

#include <QObject>

#include "../io/messages.hh"
#include "../io/remote-channel.hh"

#include "gui.hh"

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE

namespace clap {
   class RemoteGuiListener;
   class RemoteGuiFactory : public QObject, public BasicRemoteChannel::EventControl {
   public:
      RemoteGuiFactory(int socket);

      RemoteGuiFactory(void *pipeIn, void *pipeOut);

   private:
      uint32_t createClient();
      void destroyClient(uint32_t clientId);

      Gui *getClient(uint32_t clientId) const;

      template <class T>
      Gui *getClient(const T &msg) const {
         return getClient(msg.clientId);
      }

      void onMessage(const clap::RemoteChannel::Message &msg);

      void modifyFd(int flags) override;
      void removeFd() override;

      friend class RemoteGuiListener;

      struct ClientContext {
         ClientContext(uint32_t cId);
         ~ClientContext();

         const uint32_t clientId = 0;
         std::unique_ptr<RemoteGuiListener> listenner;
         std::unique_ptr<Gui> client;
      };

      uint32_t _nextClientId = 0;
      std::unordered_map<uint32_t, std::unique_ptr<ClientContext>> _guiClients;

      std::unique_ptr<clap::RemoteChannel> _channel;
      std::unique_ptr<QSocketNotifier> _socketReadNotifier;
      std::unique_ptr<QSocketNotifier> _socketWriteNotifier;
   };

} // namespace clap