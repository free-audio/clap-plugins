#pragma once

#include "../abstract-gui-factory.hh"

namespace clap {

   // The GUIs live on the main thread, so there is nothing to share between them.
   class ImGuiGuiFactory final : public AbstractGuiFactory {
   public:
      static std::shared_ptr<ImGuiGuiFactory> getInstance();

      std::unique_ptr<GuiHandle> createGui(AbstractGuiListener &listener) override;
      void releaseGui(GuiHandle &handle) override {}
   };

} // namespace clap
