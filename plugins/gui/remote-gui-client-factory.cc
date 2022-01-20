#include <QSocketNotifier>

#include "remote-gui-client-factory.hh"

namespace clap {

   RemoteGuiClientFactory::RemoteGuiClientFactory(int socket) {
      ////////////////////////
      // I/O initialization //
      ////////////////////////

#if defined(Q_OS_UNIX)
      _channel.reset(new clap::RemoteChannel(
         [this](const clap::RemoteChannel::Message &msg) { onMessage(msg); },
         false,
         *this,
         socket));

      _socketReadNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Read, this));
      connect(_socketReadNotifier.get(),
              &QSocketNotifier::activated,
              [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
                 _channel->tryReceive();
                 if (!_channel->isOpen())
                    QCoreApplication::quit();
              });

      _socketWriteNotifier.reset(new QSocketNotifier(socket, QSocketNotifier::Write, this));
      connect(_socketWriteNotifier.get(),
              &QSocketNotifier::activated,
              [this](QSocketDescriptor socket, QSocketNotifier::Type type) {
                 _channel->trySend();
                 if (!_channel->isOpen()) {
                    QCoreApplication::quit();
                 }
              });

      _socketReadNotifier->setEnabled(true);
      _socketWriteNotifier->setEnabled(false);
#elif defined(Q_OS_WINDOWS)
      _remoteChannel.reset(new clap::RemoteChannel(
         [this](const clap::RemoteChannel::Message &msg) { onMessage(msg); },
         false,
         pipeInHandle,
         pipeOutHandle,
         [] { QCoreApplication::quit(); }));
#endif
   }

   void RemoteGuiClientFactory::modifyFd(int flags) {
      _socketReadNotifier->setEnabled(flags & CLAP_POSIX_FD_READ);
      _socketWriteNotifier->setEnabled(flags & CLAP_POSIX_FD_WRITE);
   }

   void RemoteGuiClientFactory::removeFd() {
      _socketReadNotifier.reset();
      _socketWriteNotifier.reset();
      QCoreApplication::quit();
   }

   GuiClient *RemoteGuiClientFactory::getClient(uint32_t clientId) const {
      auto it = _guiClients.find(clientId);
      if (it != _guiClients.end())
         return it->second.get();
      return nullptr;
   }

   void RemoteGuiClientFactory::onMessage(const RemoteChannel::Message &msg) {
      switch (msg.type) {
      case messages::kDestroyRequest:
         messages::DestroyResponse rp;
         _channel->sendResponseAsync(rp, msg.cookie);
         QGuiApplication::quit();
         break;

      case messages::kUpdateTransportRequest: {
         messages::UpdateTransportRequest rq;
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            c->updateTransport(rq.transport);
         break;
      }

      case messages::kDefineParameterRequest: {
         messages::DefineParameterRequest rq;
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            c->defineParameter(rq.info);
         break;
      }

      case messages::kParameterValueRequest: {
         messages::ParameterValueRequest rq;
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            c->updateParameter(rq.paramId, rq.value, rq.modulation);
         break;
      }

      case messages::kSizeRequest: {
         messages::SizeRequest rq;
         messages::SizeResponse rp;
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->size(&rp.width, &rp.height);
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }

      case messages::kSetScaleRequest: {
         messages::SetScaleRequest rq;
         messages::SetScaleResponse rp{false};
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->setScale(rq.scale);
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }

      case messages::kAttachX11Request: {
         messages::AttachX11Request rq;
         messages::AttachResponse rp{false};
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->attachX11(rq.display, rq.window);
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }

      case messages::kAttachWin32Request: {
         messages::AttachWin32Request rq;
         messages::AttachResponse rp{false};
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->attachWin32(rq.hwnd);
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }

      case messages::kAttachCocoaRequest: {
         messages::AttachCocoaRequest rq;
         messages::AttachResponse rp{false};
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->attachCocoa(rq.nsView);
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }

      case messages::kShowRequest: {
         messages::ShowRequest rq;
         messages::ShowResponse rp;
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->show();
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }

      case messages::kHideRequest: {
         messages::HideRequest rq;
         messages::HideResponse rp;
         msg.get(rq);
         auto c = getClient(rq);
         if (c)
            rp.succeed = c->hide();
         _channel->sendResponseAsync(rp, msg.cookie);
         break;
      }
      }
   }
} // namespace clap