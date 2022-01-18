#pragma once

#include "gui-proxy.hh"

namespace clap {
class LocalGuiProxy : public GuiProxy {
public:
    LocalGuiProxy(AbstractGuiListener &listener);
    ~LocalGuiProxy() override;

private:

};
}
