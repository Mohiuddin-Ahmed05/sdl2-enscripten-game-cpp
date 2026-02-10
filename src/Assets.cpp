// src/Assets.cpp
#include "Assets.h"

#include <SDL2/SDL_image.h>
#include <cstdio>

SDL_Texture* loadTexture(SDL_Renderer* r, const std::string& path) {
  if (!r) {
    std::printf("loadTexture: renderer is null (%s)\n", path.c_str());
    return nullptr;
  }

  SDL_Surface* surf = IMG_Load(path.c_str());
  if (!surf) {
    std::printf("IMG_Load failed (%s): %s\n", path.c_str(), IMG_GetError());
    return nullptr;
  }

  SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
  SDL_FreeSurface(surf);

  if (!tex) {
    std::printf("SDL_CreateTextureFromSurface failed (%s): %s\n", path.c_str(), SDL_GetError());
    return nullptr;
  }

  // Helpful defaults (safe for both native + web)
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

  return tex;
}

void destroyTexture(SDL_Texture*& tex) {
  if (tex) {
    SDL_DestroyTexture(tex);
    tex = nullptr;
  }
}

void drawTexture(SDL_Renderer* r, SDL_Texture* tex, const SDL_FRect& dst) {
  if (!r || !tex) return;
  SDL_RenderCopyF(r, tex, nullptr, &dst);
}
