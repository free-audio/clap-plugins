#include "abstract-gui.hh"

namespace clap {

   AbstractGui::AbstractGui(AbstractGuiListener &listener) : _listener(listener) {}
   AbstractGui::~AbstractGui() = default;

} // namespace clap