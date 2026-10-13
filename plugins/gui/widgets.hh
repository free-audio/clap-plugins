#pragma once

#include <clap/clap.h>

namespace clap {

   class ImGuiGui;

   // Draws a knob of the given size, with the parameter's name below it.
   //
   // Drag vertically to adjust, double-click to reset to the default value.
   void knob(ImGuiGui &gui, clap_id paramId, float size);

   // Height of the knob() widget
   float knobHeight(float size);

} // namespace clap
