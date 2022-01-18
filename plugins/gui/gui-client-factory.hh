#pragma once

#include <memory>
#include <unordered_map>

#include <QThread>
#include <QObject>

class QGuiApplication;
class QThread;

namespace clap {

class GuiClient;
class AbstractGui;

class GuiClientFactory : public QObject
{
    using super = QObject;

    Q_OBJECT

public:
    ~GuiClientFactory();
    static std::shared_ptr<GuiClientFactory> getInstance();

    std::shared_ptr<AbstractGui> createGuiClient();

private:
    GuiClientFactory();

    void doCreateGuiClient();

    static std::weak_ptr<GuiClientFactory> _instance;

    std::unique_ptr<QThread> _thread;
    std::unique_ptr<QGuiApplication> _app;
    std::unordered_map<uint32_t, std::unique_ptr<GuiClient>> _guiClients;
};

}