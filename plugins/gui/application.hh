#pragma once

#include <QApplication>
#include <QSocketNotifier>
#include <QWindow>

#include "../io/remote-channel.hh"
#include "plugin-proxy.hh"
#include "transport-proxy.hh"

class QQuickView;

class Application : public QApplication, public clap::RemoteChannel::EventControl {
   Q_OBJECT;
   using super = QApplication;

public:
   Application(int& argc, char **argv);

   clap::RemoteChannel& remoteChannel() const { return *_remoteChannel; }
   void modifyFd(clap_fd_flags flags) override;
   void removeFd() override;

   static Application& instance() { return *dynamic_cast<Application *>(QApplication::instance()); }

private:
   void onMessage(const clap::RemoteChannel::Message& msg);

   QQuickView *_quickView = nullptr;
   std::unique_ptr<QSocketNotifier> _socketReadNotifier;
   std::unique_ptr<QSocketNotifier> _socketWriteNotifier;
   std::unique_ptr<QSocketNotifier> _socketErrorNotifier;

   std::unique_ptr<QWindow> _hostWindow = nullptr;

   std::unique_ptr<clap::RemoteChannel> _remoteChannel;
   PluginProxy *_pluginProxy = nullptr;
   TransportProxy *_transportProxy = nullptr;
};