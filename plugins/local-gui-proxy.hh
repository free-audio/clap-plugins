#pragma once

#include "gui-proxy.hh"

namespace clap {
class LocalGuiProxy : public GuiProxy {
public:
    LocalGuiProxy(CorePlugin &plugin);
    ~LocalGuiProxy() override;

private:

};
}
