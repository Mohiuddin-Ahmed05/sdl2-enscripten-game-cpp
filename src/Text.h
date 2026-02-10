// src/Text.h
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void drawTextCentered(
  SDL_Renderer* renderer,
  TTF_Font* font,
  const char* text,
  const SDL_FRect& box
);

// Overload with custom color
void drawTextCentered(
  SDL_Renderer* renderer,
  TTF_Font* font,
  const char* text,
  const SDL_FRect& box,
  SDL_Color color
);
