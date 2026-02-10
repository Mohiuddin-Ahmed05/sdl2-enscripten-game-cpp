// src/Zoom.h
#pragma once

#include <SDL2/SDL.h>

namespace zoom {

// Basic camera/zoom state for converting world -> screen coordinates.
// - camX is in world units
// - zoom is a multiplier (1.0 = no zoom, 1.25 = 25% bigger)
struct Camera {
  float camX = 0.0f;
  float zoom = 1.0f;

  // Optional: anchor the world to a baseline (useful for platformers).
  // If enabled, y is transformed relative to anchorWorldY so the "ground"
  // can visually stay consistent when zoom changes.
  bool  useAnchorY = false;
  float anchorWorldY = 0.0f;
};

// Convert a world-space float rect to a screen-space float rect.
SDL_FRect worldToScreen(const Camera& c, const SDL_FRect& world);

// Convert a world-space int rect to a screen-space int rect (rounded).
SDL_Rect worldToScreenI(const Camera& c, const SDL_FRect& world);

// Helper: convert a world x coordinate to screen x.
float worldXToScreen(const Camera& c, float worldX);

// Helper: convert a world y coordinate to screen y.
float worldYToScreen(const Camera& c, float worldY);

} // namespace zoom
