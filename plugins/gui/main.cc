#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QThread>
#include <QCommandLineParser>

#include "gui-client.hh"
#include "remote-gui-listener.h"

int main(int argc, char **argv) {
   /* Useful to attach child process with debuggers which don't support follow childs */
   bool waitForDebbugger = false;
   while (waitForDebbugger)
      QThread::sleep(1);

   QGuiApplication app(argc, argv);

   std::unique_ptr<clap::RemoteGuiListener> remoteGuiListener;


   qmlRegisterType<clap::ParameterProxy>("org.clap", 1, 0, "ParameterProxy");
   qmlRegisterType<clap::TransportProxy>("org.clap", 1, 0, "TransportProxy");
   qmlRegisterType<clap::PluginProxy>("org.clap", 1, 0, "PluginProxy");

   QCommandLineParser parser;

   QCommandLineOption skinOpt("skin", QObject::tr("path to the skin directory"), QObject::tr("path"));
   QCommandLineOption qmlLibOpt("qml-import", QObject::tr("QML import path"), QObject::tr("path"));

#ifdef Q_OS_UNIX
   QCommandLineOption socketOpt("socket", QObject::tr("socket fd"), QObject::tr("path"));
   parser.addOption(socketOpt);
#endif

#ifdef Q_OS_WINDOWS
   QCommandLineOption pipeInOpt("pipe-in", tr("input pipe handle"), tr("path"));
   QCommandLineOption pipeOutOpt("pipe-out", tr("output pipe handle"), tr("path"));
   parser.addOption(pipeInOpt);
   parser.addOption(pipeOutOpt);
#endif

   parser.addOption(skinOpt);
   parser.addOption(qmlLibOpt);
   parser.addHelpOption();

   parser.process(app);

#if defined(Q_OS_UNIX)
   auto socket = parser.value(socketOpt).toULongLong();
   remoteGuiListener = std::make_unique<clap::RemoteGuiListener>(socket);
#elif defined(Q_OS_WINDOWS)
   auto pipeInName = parser.value(pipeInOpt).toStdString();
   auto pipeOutName = parser.value(pipeOutOpt).toStdString();

   auto pipeInHandle = CreateFileA(pipeInName.c_str(),
                                   GENERIC_READ,
                                   0,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                   NULL);

   auto pipeOutHandle = CreateFileA(pipeOutName.c_str(),
                                    GENERIC_WRITE,
                                    0,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                    NULL);
   remoteGuiListener = std::make_unique<clap::RemoteGuiListener>(pipeInHandle, pipeOutHandle);
#endif

   clap::GuiClient client(*remoteGuiListener, parser.values(qmlLibOpt), QUrl::fromLocalFile(parser.value(skinOpt) + "/main.qml"));

   /* Run the app */
   return app.exec();
}