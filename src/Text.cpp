// src/Text.cpp
#include "Text.h"

static void drawTextCenteredImpl(
  SDL_Renderer* renderer,
  TTF_Font* font,
  const char* text,
  const SDL_FRect& box,
  SDL_Color color
) {
  if (!renderer || !font || !text || !text[0]) return;

  SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
  if (!surf) return;

  SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
  if (!tex) {
    SDL_FreeSurface(surf);
    return;
  }

  SDL_FRect dst;
  dst.w = (float)surf->w;
  dst.h = (float)surf->h;
  dst.x = box.x + (box.w - dst.w) * 0.5f;
  dst.y = box.y + (box.h - dst.h) * 0.5f;

  SDL_FreeSurface(surf);

  SDL_RenderCopyF(renderer, tex, nullptr, &dst);
  SDL_DestroyTexture(tex);
}

void drawTextCentered(
  SDL_Renderer* renderer,
  TTF_Font* font,
  const char* text,
  const SDL_FRect& box
) {
  // Default: light text for dark HUD areas
  SDL_Color color { 230, 235, 245, 255 };
  drawTextCenteredImpl(renderer, font, text, box, color);
}

void drawTextCentered(
  SDL_Renderer* renderer,
  TTF_Font* font,
  const char* text,
  const SDL_FRect& box,
  SDL_Color color
) {
  drawTextCenteredImpl(renderer, font, text, box, color);
}
