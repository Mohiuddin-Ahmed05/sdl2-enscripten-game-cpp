// src/Assets.h
#pragma once

#include <SDL2/SDL.h>
#include <string>

SDL_Texture* loadTexture(SDL_Renderer* r, const std::string& path);
void destroyTexture(SDL_Texture*& tex);

void drawTexture(SDL_Renderer* r, SDL_Texture* tex, const SDL_FRect& dst);
