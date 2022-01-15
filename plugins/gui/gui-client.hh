#pragma once

#include <QGuiApplication>
#include <QSocketNotifier>
#include <QWindow>

#include "../io/remote-channel.hh"
#include "plugin-proxy.hh"
#include "transport-proxy.hh"

class QQuickView;

class GuiClient : public QObject, public clap::RemoteChannel::EventControl {
   Q_OBJECT;
   using super = QGuiApplication;

public:
   GuiClient(int socket, const QStringList& qmlImportPath, const QString& qmlSkin);
   GuiClient(void* pipeIn, void *pipeOut, const QStringList& qmlImportPath, const QString& qmlSkin);

   clap::RemoteChannel& remoteChannel() const { return *_remoteChannel; }
   void modifyFd(int flags) override;
   void removeFd() override;

private:
   void onMessage(const clap::RemoteChannel::Message& msg);

   QQuickView *_quickView = nullptr;
   std::unique_ptr<QSocketNotifier> _socketReadNotifier;
   std::unique_ptr<QSocketNotifier> _socketWriteNotifier;

   std::unique_ptr<QWindow> _hostWindow = nullptr;

   std::unique_ptr<clap::RemoteChannel> _remoteChannel;
   PluginProxy *_pluginProxy = nullptr;
   TransportProxy *_transportProxy = nullptr;
};