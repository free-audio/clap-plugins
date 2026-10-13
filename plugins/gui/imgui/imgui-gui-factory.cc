#include "imgui-gui-factory.hh"
#include "imgui-gui.hh"

namespace clap {

   std::shared_ptr<ImGuiGuiFactory> ImGuiGuiFactory::getInstance() {
      static auto instance = std::make_shared<ImGuiGuiFactory>();
      return instance;
   }

   std::unique_ptr<GuiHandle> ImGuiGuiFactory::createGui(AbstractGuiListener &listener) {
      return std::make_unique<GuiHandle>(getInstance(), std::make_shared<ImGuiGui>(listener));
   }

} // namespace clap
