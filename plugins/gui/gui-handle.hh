#pragma once

#include <memory>

namespace clap {

class AbstractGui;
class AbstractGuiFactory;

class GuiHandle final {
public:
   GuiHandle(const std::shared_ptr<AbstractGuiFactory>& factory, const std::shared_ptr<AbstractGui>& gui);
   ~GuiHandle();

   GuiHandle(const GuiHandle &other) noexcept = delete;
   GuiHandle(GuiHandle &&other) noexcept = delete;
   GuiHandle &operator=(const GuiHandle &other) noexcept = delete;
   GuiHandle &operator=(GuiHandle &&other) noexcept = delete;

   auto &gui() const { return *_gui; }

private:
   std::shared_ptr<AbstractGuiFactory> _factory;
   std::shared_ptr<AbstractGui> _gui;
};

} // namespace clap
