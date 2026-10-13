#pragma once

#include <string>

#include "imgui-gui.hh"

namespace clap {

   // Returns the skin with the given name, or the generic one which shows all the parameters.
   const Skin &findSkin(const std::string &name);

} // namespace clap
