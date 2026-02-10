// src/Game.cpp
#include "Game.h"

#include <cstdio>
#include <utility>

#include "Scene.h"
#include "MenuScene.h"
#include "OptionsScene.h"
#include "GameScene.h"

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
#endif

Game::Game(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font)
  : m_window(window), m_renderer(renderer), m_font(font) {
  setScene(SceneId::Menu);
}

Game::~Game() {
  if (m_renderer) {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }
}

void Game::requestQuit() { m_running = false; }

void Game::requestScene(SceneId next) {
  m_pendingId = next;
  m_hasPendingSceneChange = true;
}

void Game::getRenderSize(int& w, int& h) const {
  w = 0; h = 0;
  if (m_renderer) SDL_GetRendererOutputSize(m_renderer, &w, &h);
}

std::unique_ptr<Scene> Game::makeScene(SceneId id) {
  switch (id) {
    case SceneId::Menu:    return std::make_unique<MenuScene>(this);
    case SceneId::Play:    return std::make_unique<GameScene>(this);
    case SceneId::Options: return std::make_unique<OptionsScene>(this);
    default:               return std::make_unique<MenuScene>(this);
  }
}

void Game::setScene(SceneId id) {
  m_currentId = id;
  m_scene = makeScene(id);

  // If the renderer was recreated before this scene was constructed,
  // it will load textures against current renderer in its constructor.
  // No action required here.
}

// ---------------- Display controls ----------------

void Game::toggleFullscreen() {
  if (!m_window) return;

#ifdef __EMSCRIPTEN__
  // Web fullscreen is handled differently; avoid renderer recreation.
  // You can wire an HTML button + Emscripten fullscreen APIs later.
  return;
#else
  m_isFullscreen = !m_isFullscreen;

  Uint32 flags = m_isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
  if (SDL_SetWindowFullscreen(m_window, flags) != 0) {
    std::printf("SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
    m_isFullscreen = !m_isFullscreen;
    return;
  }

  m_rendererDirty = true;
#endif
}

void Game::setWindowedResolution(int w, int h) {
  if (!m_window) return;

#ifdef __EMSCRIPTEN__
  // Canvas sizing is controlled by HTML/CSS; do nothing in web build.
  (void)w; (void)h;
  return;
#else
  if (m_isFullscreen) return;

  SDL_SetWindowSize(m_window, w, h);
  SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  m_rendererDirty = true;
#endif
}

void Game::applyDisplayChanges() {
  if (!m_rendererDirty) return;
  m_rendererDirty = false;

  if (!m_window) return;

#ifdef __EMSCRIPTEN__
  // Avoid renderer destruction/recreation in web build.
  return;
#else
  if (m_renderer) {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }

  m_renderer = SDL_CreateRenderer(
    m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if (!m_renderer) {
    std::printf("SDL_CreateRenderer failed after display change: %s\n", SDL_GetError());
    requestQuit();
    return;
  }

  // IMPORTANT: notify active scene so it can reload textures
  if (m_scene) m_scene->onRendererChanged(m_renderer);
#endif
}

// --------------------------------------------------

void Game::handleEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT) {
    requestQuit();
    return;
  }

  if (e.type == SDL_KEYDOWN && e.key.repeat == 0 && e.key.keysym.sym == SDLK_ESCAPE) {
    if (m_currentId == SceneId::Menu) requestQuit();
    else requestScene(SceneId::Menu);
    return;
  }

  if (m_scene) m_scene->handleEvent(e);
}

void Game::update(float dt) {
  if (m_scene) m_scene->update(dt);

  if (m_hasPendingSceneChange) {
    m_hasPendingSceneChange = false;
    setScene(m_pendingId);
  }
}

void Game::render() {
  if (m_scene) m_scene->render(m_renderer);
}

bool Game::isRunning() const {
  return m_running;
}

// One frame (Emscripten-safe)
void Game::tick() {
  if (!m_renderer) {
    std::printf("Game::tick(): renderer is null\n");
    requestQuit();
    return;
  }

  static Uint64 prev = SDL_GetPerformanceCounter();
  Uint64 now = SDL_GetPerformanceCounter();
  const double freq = (double)SDL_GetPerformanceFrequency();
  float dt = (float)((now - prev) / freq);
  prev = now;

  SDL_Event e{};
  while (SDL_PollEvent(&e)) {
    handleEvent(e);
    if (!m_running) break;
  }
  if (!m_running) return;

  applyDisplayChanges();
  if (!m_running || !m_renderer) return;

  update(dt);
  render();

  SDL_RenderPresent(m_renderer);
}

void Game::run() {
#ifndef __EMSCRIPTEN__
  if (!m_renderer) {
    std::printf("Game::run(): renderer is null\n");
    return;
  }

  Uint64 prev = SDL_GetPerformanceCounter();
  const double freq = (double)SDL_GetPerformanceFrequency();
  SDL_Event e{};

  while (m_running) {
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)((now - prev) / freq);
    prev = now;

    while (SDL_PollEvent(&e)) {
      handleEvent(e);
      if (!m_running) break;
    }
    if (!m_running) break;

    applyDisplayChanges();
    if (!m_running || !m_renderer) break;

    update(dt);
    render();

    SDL_RenderPresent(m_renderer);
  }
#else
  // In web builds, main.cpp drives tick() using emscripten_set_main_loop()
  std::printf("Game::run() is not used in Emscripten builds.\n");
#endif
}
