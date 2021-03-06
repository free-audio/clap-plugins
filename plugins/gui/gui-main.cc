#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSocketNotifier>
#include <QThread>

#ifdef Q_OS_UNIX
#   include <signal.h>
#endif

#include "remote-gui-factory.hh"

int main(int argc, char **argv) {
#ifdef Q_OS_UNIX
   ::signal(SIGPIPE, SIG_IGN);
#endif

#ifdef Q_OS_LINUX
   ::setenv("QT_QPA_PLATFORM", "xcb", 1);
#endif

   /* Useful to attach child process with debuggers which don't support follow childs */
   bool waitForDebbugger = false;
   while (waitForDebbugger)
      QThread::sleep(1);

   QGuiApplication::setAttribute(Qt::AA_PluginApplication);
   QGuiApplication app(argc, argv);

   qmlRegisterType<clap::ParameterProxy>("org.clap", 1, 0, "ParameterProxy");
   qmlRegisterType<clap::TransportProxy>("org.clap", 1, 0, "TransportProxy");
   qmlRegisterType<clap::PluginProxy>("org.clap", 1, 0, "PluginProxy");

   QCommandLineParser parser;

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

   parser.addHelpOption();

   parser.process(app);

#if defined(Q_OS_UNIX)
   auto socket = parser.value(socketOpt).toULongLong();
   clap::RemoteGuiFactory factory(socket);
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
   clap::RemoteGuiClientFactory factory(pipeInHandle, pipeOutHandle);
#endif

   /* Run the app */
   return app.exec();
}