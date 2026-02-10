// src/Game.h
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>

// Forward declarations
class Scene;

class Game {
public:
  enum class SceneId { Menu, Play, Options };

  Game(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font);
  ~Game();

  // Native loop (NOT used in Emscripten builds)
  void run();

  // One-frame step (used by Emscripten main loop)
  void tick();

  bool isRunning() const;

  void requestQuit();
  void requestScene(SceneId next);

  SDL_Renderer* renderer() const { return m_renderer; }
  TTF_Font* font() const { return m_font; }
  void getRenderSize(int& w, int& h) const;

  // Window access for display settings
  SDL_Window* window() const { return m_window; }

  // Display controls
  bool isFullscreen() const { return m_isFullscreen; }
  void toggleFullscreen();                  // desktop fullscreen (no-op on web)
  void setWindowedResolution(int w, int h); // only applies if not fullscreen (no-op on web)
  void applyDisplayChanges();               // rebuild renderer when needed (native only)

  // Optional helper: allow scenes to request a renderer rebuild (if needed)
  void markRendererDirty() { m_rendererDirty = true; }

private:
  void handleEvent(const SDL_Event& e);
  void update(float dt);
  void render();

  void setScene(SceneId id);
  std::unique_ptr<Scene> makeScene(SceneId id);

private:
  SDL_Window*   m_window   = nullptr; // not owned (created/destroyed in main)
  SDL_Renderer* m_renderer = nullptr; // owned by Game if it recreates it
  TTF_Font*     m_font     = nullptr; // not owned

  bool m_running = true;

  SceneId m_currentId = SceneId::Menu;
  SceneId m_pendingId = SceneId::Menu;
  bool    m_hasPendingSceneChange = false;

  std::unique_ptr<Scene> m_scene;

  // Display state
  bool m_isFullscreen = false;
  bool m_rendererDirty = false;
};
