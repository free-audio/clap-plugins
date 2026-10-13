#pragma once

#include "platform-window.hh"

struct ImDrawData;

namespace clap {

   // Renders Dear ImGui's draw data into a pixel buffer.
   //
   // Our GUIs are small, so the CPU is fast enough. In exchange we don't need any GPU context,
   // which is a common source of trouble inside a host, and every platform only has to show a
   // pixel buffer.
   class SoftwareRenderer {
   public:
      // Handles the texture requests (ImGuiBackendFlags_RendererHasTextures), then renders.
      static void render(ImDrawData &drawData, Framebuffer &framebuffer, uint32_t clearColor);
   };

} // namespace clap
