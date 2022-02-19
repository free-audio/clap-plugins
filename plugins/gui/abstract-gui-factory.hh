#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gui-handle.hh"

namespace clap {

   class AbstractGui;
   class AbstractGuiListener;
   class AbstractGuiFactory {
   public:
      virtual ~AbstractGuiFactory();

      virtual std::unique_ptr<GuiHandle>
      createGui(AbstractGuiListener &listener) = 0;

      virtual void releaseGui(GuiHandle &handle) = 0;
   };

} // namespace clap