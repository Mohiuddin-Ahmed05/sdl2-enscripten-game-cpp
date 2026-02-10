// src/zoom.cpp
#include "Zoom.h"

#include <cmath>

namespace zoom {

static inline float applyAnchorY(const Camera& c, float worldY) {
  if (!c.useAnchorY) return worldY;
  // When anchored, measure y relative to anchorWorldY, then scale.
  // This can help keep "ground" visually stable depending on how you choose anchorWorldY.
  return (worldY - c.anchorWorldY);
}

SDL_FRect worldToScreen(const Camera& c, const SDL_FRect& world) {
  SDL_FRect out{};

  // X is always camera-relative
  out.x = (world.x - c.camX) * c.zoom;

  // Y can be anchored or not
  const float yBase = applyAnchorY(c, world.y);
  out.y = yBase * c.zoom;

  out.w = world.w * c.zoom;
  out.h = world.h * c.zoom;

  return out;
}

SDL_Rect worldToScreenI(const Camera& c, const SDL_FRect& world) {
  SDL_FRect f = worldToScreen(c, world);
  SDL_Rect out{};
  out.x = (int)std::lround(f.x);
  out.y = (int)std::lround(f.y);
  out.w = (int)std::lround(f.w);
  out.h = (int)std::lround(f.h);
  return out;
}

float worldXToScreen(const Camera& c, float worldX) {
  return (worldX - c.camX) * c.zoom;
}

float worldYToScreen(const Camera& c, float worldY) {
  const float yBase = applyAnchorY(c, worldY);
  return yBase * c.zoom;
}

} // namespace zoom
