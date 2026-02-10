// src/Scene.h
#pragma once
#include <SDL2/SDL.h>

class Scene {
public:
  virtual ~Scene() = default;

  // Event-based input (no polling here unless you want it)
  virtual void handleEvent(const SDL_Event& e) = 0;

  // Per-frame update
  virtual void update(float dt) = 0;

  // Draw scene content. Game() will call SDL_RenderPresent().
  virtual void render(SDL_Renderer* r) = 0;

  // IMPORTANT:
  // Called when Game recreates the SDL_Renderer (fullscreen / resize).
  // Default is no-op so scenes without textures don't care.
  virtual void onRendererChanged(SDL_Renderer*) {}
};
