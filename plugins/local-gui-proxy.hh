#pragma once

#include "remote-gui.hh"

namespace clap {
class ThreadedRemoteGui : public RemoteGui {
public:
    ThreadedRemoteGui(CorePlugin &plugin);
    ~ThreadedRemoteGui() override;
};
}
