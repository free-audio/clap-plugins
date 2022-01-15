#pragma once

#include <memory>
#include <unordered_map>

#include <QThread>
#include <QObject>

class QGuiApplication;
class QThread;
class GuiClient;

class GuiClientFactory : public QObject
{
    using super = QObject;

    Q_OBJECT

public:
    ~GuiClientFactory();
    static std::shared_ptr<GuiClientFactory> getInstance();

    uint32_t createGuiClient();
    void terminteGuiClient(uint32_t clientId);

private:
    GuiClientFactory();

    void doCreateGuiClient();

    static std::weak_ptr<GuiClientFactory> _instance;

    std::unique_ptr<QThread> _thread;
    std::unique_ptr<QGuiApplication> _app;
    std::unordered_map<uint32_t, std::unique_ptr<GuiClient>> _guiClients;
};