#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <imgui.h>

#include "software-renderer.hh"

namespace clap {

   namespace {
      // Vertex positions are snapped to 1/16th of a pixel
      constexpr int kSubPixelBits = 4;
      constexpr int64_t kSubPixels = 1 << kSubPixelBits;

      // [x0, x1) x [y0, y1)
      struct ClipRect {
         int x0, y0, x1, y1;
      };

      struct Point {
         int64_t x, y;
      };

      struct Color {
         float r, g, b, a; // 0..255
      };

      int64_t orient(const Point &a, const Point &b, int64_t x, int64_t y) {
         return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
      }

      // A pixel exactly on an edge shared by two triangles must be drawn once, otherwise the
      // anti-aliasing fringes would be blended twice. Both triangles are normalized to the same
      // winding, so the shared edge goes in opposite directions and only one of them owns it.
      bool ownsEdge(const Point &a, const Point &b) {
         const auto dx = b.x - a.x;
         const auto dy = b.y - a.y;
         return dy < 0 || (dy == 0 && dx > 0);
      }

      Color unpack(ImU32 c) {
         return {float(c & 0xff), float((c >> 8) & 0xff), float((c >> 16) & 0xff), float(c >> 24)};
      }

      Color sample(const ImTextureData *tex, float u, float v) {
         if (!tex)
            return {255, 255, 255, 255};

         // The font atlas is rendered at the framebuffer density, so the nearest texel is exact
         const int x = std::clamp(int(u * tex->Width), 0, tex->Width - 1);
         const int y = std::clamp(int(v * tex->Height), 0, tex->Height - 1);
         const auto *p = tex->Pixels + (size_t(y) * tex->Width + x) * tex->BytesPerPixel;

         if (tex->Format == ImTextureFormat_Alpha8)
            return {255, 255, 255, float(p[0])};
         return {float(p[0]), float(p[1]), float(p[2]), float(p[3])};
      }

      Color modulate(const Color &a, const Color &b) {
         constexpr float k = 1.f / 255.f;
         return {a.r * b.r * k, a.g * b.g * k, a.b * b.b * k, a.a * b.a * k};
      }

      void blend(uint32_t &dst, const Color &src) {
         if (src.a <= 0.f)
            return;

         if (src.a >= 255.f) {
            dst = (uint32_t(src.r) << 16) | (uint32_t(src.g) << 8) | uint32_t(src.b);
            return;
         }

         const float a = src.a * (1.f / 255.f);
         float r = (dst >> 16) & 0xff;
         float g = (dst >> 8) & 0xff;
         float b = dst & 0xff;
         r += (src.r - r) * a;
         g += (src.g - g) * a;
         b += (src.b - b) * a;
         dst = (uint32_t(r + .5f) << 16) | (uint32_t(g + .5f) << 8) | uint32_t(b + .5f);
      }

      void drawTriangle(Framebuffer &fb,
                        const ClipRect &clip,
                        const ImDrawVert *v[3],
                        const ImTextureData *tex,
                        ImVec2 offset,
                        ImVec2 scale) {
         Point p[3];
         for (int i = 0; i < 3; ++i) {
            p[i].x = std::llround((v[i]->pos.x - offset.x) * scale.x * kSubPixels);
            p[i].y = std::llround((v[i]->pos.y - offset.y) * scale.y * kSubPixels);
         }

         int64_t area = orient(p[0], p[1], p[2].x, p[2].y);
         if (area == 0)
            return;

         if (area < 0) {
            std::swap(p[1], p[2]);
            std::swap(v[1], v[2]);
            area = -area;
         }

         // Pixels whose center is in the bounding box, clipped
         const auto minX = std::min({p[0].x, p[1].x, p[2].x});
         const auto maxX = std::max({p[0].x, p[1].x, p[2].x});
         const auto minY = std::min({p[0].y, p[1].y, p[2].y});
         const auto maxY = std::max({p[0].y, p[1].y, p[2].y});
         const int x0 = std::max<int64_t>(clip.x0, minX >> kSubPixelBits);
         const int x1 = std::min<int64_t>(clip.x1, (maxX + kSubPixels - 1) >> kSubPixelBits);
         const int y0 = std::max<int64_t>(clip.y0, minY >> kSubPixelBits);
         const int y1 = std::min<int64_t>(clip.y1, (maxY + kSubPixels - 1) >> kSubPixelBits);
         if (x0 >= x1 || y0 >= y1)
            return;

         // w0, w1 and w2 are the edge functions, proportional to the barycentric coordinates of
         // v[0], v[1] and v[2]. The bias excludes the pixels on the edges we don't own.
         const int64_t bias0 = ownsEdge(p[1], p[2]) ? 0 : -1;
         const int64_t bias1 = ownsEdge(p[2], p[0]) ? 0 : -1;
         const int64_t bias2 = ownsEdge(p[0], p[1]) ? 0 : -1;

         const int64_t dx0 = (p[1].y - p[2].y) * kSubPixels;
         const int64_t dx1 = (p[2].y - p[0].y) * kSubPixels;
         const int64_t dx2 = (p[0].y - p[1].y) * kSubPixels;
         const int64_t dy0 = (p[2].x - p[1].x) * kSubPixels;
         const int64_t dy1 = (p[0].x - p[2].x) * kSubPixels;
         const int64_t dy2 = (p[1].x - p[0].x) * kSubPixels;

         const int64_t cx = x0 * kSubPixels + kSubPixels / 2;
         const int64_t cy = y0 * kSubPixels + kSubPixels / 2;
         int64_t row0 = orient(p[1], p[2], cx, cy) + bias0;
         int64_t row1 = orient(p[2], p[0], cx, cy) + bias1;
         int64_t row2 = orient(p[0], p[1], cx, cy) + bias2;

         const bool hasUniformColor = v[0]->col == v[1]->col && v[1]->col == v[2]->col;
         const bool hasUniformUv = v[0]->uv.x == v[1]->uv.x && v[1]->uv.x == v[2]->uv.x &&
                                   v[0]->uv.y == v[1]->uv.y && v[1]->uv.y == v[2]->uv.y;

         // Solid fills sample the texture's white pixel, so that's the common case
         Color uniform{};
         if (hasUniformColor && hasUniformUv)
            uniform = modulate(unpack(v[0]->col), sample(tex, v[0]->uv.x, v[0]->uv.y));

         const Color c[3] = {unpack(v[0]->col), unpack(v[1]->col), unpack(v[2]->col)};
         const float invArea = 1.f / float(area);

         for (int y = y0; y < y1; ++y) {
            int64_t w0 = row0;
            int64_t w1 = row1;
            int64_t w2 = row2;
            uint32_t *line = fb.pixels + size_t(y) * fb.stride;

            for (int x = x0; x < x1; ++x, w0 += dx0, w1 += dx1, w2 += dx2) {
               if ((w0 | w1 | w2) < 0)
                  continue;

               if (hasUniformColor && hasUniformUv) {
                  blend(line[x], uniform);
                  continue;
               }

               const float l1 = float(w1 - bias1) * invArea;
               const float l2 = float(w2 - bias2) * invArea;
               const float l0 = 1.f - l1 - l2;

               const Color col = {l0 * c[0].r + l1 * c[1].r + l2 * c[2].r,
                                  l0 * c[0].g + l1 * c[1].g + l2 * c[2].g,
                                  l0 * c[0].b + l1 * c[1].b + l2 * c[2].b,
                                  l0 * c[0].a + l1 * c[1].a + l2 * c[2].a};
               const float u = l0 * v[0]->uv.x + l1 * v[1]->uv.x + l2 * v[2]->uv.x;
               const float t = l0 * v[0]->uv.y + l1 * v[1]->uv.y + l2 * v[2]->uv.y;
               blend(line[x], modulate(col, sample(tex, u, t)));
            }

            row0 += dy0;
            row1 += dy1;
            row2 += dy2;
         }
      }

      void updateTexture(ImTextureData &tex) {
         switch (tex.Status) {
         case ImTextureStatus_WantCreate:
         case ImTextureStatus_WantUpdates:
            // ImGui keeps the pixels in memory, and that's all we need
            tex.SetTexID(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(&tex)));
            tex.SetStatus(ImTextureStatus_OK);
            break;

         case ImTextureStatus_WantDestroy:
            if (tex.UnusedFrames > 0) {
               tex.SetTexID(ImTextureID_Invalid);
               tex.SetStatus(ImTextureStatus_Destroyed);
            }
            break;

         default:
            break;
         }
      }
   } // namespace

   void SoftwareRenderer::render(ImDrawData &drawData, Framebuffer &fb, uint32_t clearColor) {
      if (drawData.Textures) {
         for (auto *tex : *drawData.Textures)
            updateTexture(*tex);
      }

      for (int y = 0; y < fb.height; ++y)
         std::fill_n(fb.pixels + size_t(y) * fb.stride, fb.width, clearColor);

      const ImVec2 offset = drawData.DisplayPos;
      const ImVec2 scale = drawData.FramebufferScale;

      for (const ImDrawList *list : drawData.CmdLists) {
         for (const ImDrawCmd &cmd : list->CmdBuffer) {
            if (cmd.UserCallback) {
               // We have no render state to reset
               if (cmd.UserCallback != ImGui::GetPlatformIO().DrawCallback_ResetRenderState)
                  cmd.UserCallback(list, &cmd);
               continue;
            }

            auto toPixel = [](float v, int max) { return int(std::clamp(v, 0.f, float(max))); };
            const ClipRect clip = {
               toPixel(std::floor((cmd.ClipRect.x - offset.x) * scale.x), fb.width),
               toPixel(std::floor((cmd.ClipRect.y - offset.y) * scale.y), fb.height),
               toPixel(std::ceil((cmd.ClipRect.z - offset.x) * scale.x), fb.width),
               toPixel(std::ceil((cmd.ClipRect.w - offset.y) * scale.y), fb.height),
            };
            if (clip.x0 >= clip.x1 || clip.y0 >= clip.y1)
               continue;

            const auto *tex = reinterpret_cast<const ImTextureData *>(
               static_cast<intptr_t>(cmd.GetTexID()));

            const ImDrawIdx *idx = list->IdxBuffer.Data + cmd.IdxOffset;
            const ImDrawVert *vtx = list->VtxBuffer.Data + cmd.VtxOffset;
            for (unsigned i = 0; i + 2 < cmd.ElemCount; i += 3) {
               const ImDrawVert *v[3] = {&vtx[idx[i]], &vtx[idx[i + 1]], &vtx[idx[i + 2]]};
               drawTriangle(fb, clip, v, tex, offset, scale);
            }
         }
      }
   }

} // namespace clap
