#include "remote-gui-client-factory.hh" clap::RemoteGuiClientFactory::RemoteGuiClientFactory(int socket)
{}
clap::GuiClient *clap::RemoteGuiClientFactory::getClient(uint32_t clientId) const {
   auto it = _guiClients.find(clientId);
   if (it != _guiClients.end())
      return it->second.get();
   return nullptr;
}
void clap::RemoteGuiClientFactory::onMessage(const clap::RemoteChannel::Message &msg) {
   switch (msg.type) {
   case clap::messages::kDestroyRequest:
      clap::messages::DestroyResponse rp;
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      QGuiApplication::quit();
      break;

   case clap::messages::kUpdateTransportRequest: {
      clap::messages::UpdateTransportRequest rq;
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         c->updateTransport(rq.transport);
      break;
   }

   case clap::messages::kDefineParameterRequest: {
      clap::messages::DefineParameterRequest rq;
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         c->defineParameter(rq.info);
      break;
   }

   case clap::messages::kParameterValueRequest: {
      clap::messages::ParameterValueRequest rq;
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         c->updateParameter(rq.paramId, rq.value, rq.modulation);
      break;
   }

   case clap::messages::kSizeRequest: {
      clap::messages::SizeRequest rq;
      clap::messages::SizeResponse rp;
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->size(&rp.width, &rp.height);
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kSetScaleRequest: {
      clap::messages::SetScaleRequest rq;
      clap::messages::SetScaleResponse rp{false};
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->setScale(rq.scale);
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachX11Request: {
      clap::messages::AttachX11Request rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->attachX11(rq.display, rq.window);
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachWin32Request: {
      clap::messages::AttachWin32Request rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->attachWin32(rq.hwnd);
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kAttachCocoaRequest: {
      clap::messages::AttachCocoaRequest rq;
      clap::messages::AttachResponse rp{false};
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->attachCocoa(rq.nsView);
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kShowRequest: {
      clap::messages::ShowRequest rq;
      clap::messages::ShowResponse rp;
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->show();
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }

   case clap::messages::kHideRequest: {
      clap::messages::HideRequest rq;
      clap::messages::HideResponse rp;
      msg.get(rq);
      auto c = getClient(rq);
      if (c)
         rp.succeed = c->hide();
      _remoteChannel->sendResponseAsync(rp, msg.cookie);
      break;
   }
   }
}
