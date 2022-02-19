#include "gui-handle.hh"
#include "abstract-gui-factory.hh"
#include "abstract-gui.hh"

namespace clap {

   GuiHandle::GuiHandle(const std::shared_ptr<AbstractGuiFactory> &factory,
                        const std::shared_ptr<AbstractGui> &gui)
      : _factory(factory), _gui(gui) {}

   GuiHandle::~GuiHandle() {
      _factory->releaseGui(*this);
      _gui->destroy();
   }

} // namespace clap