// src/main.cpp
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <cstdio>

#include "Game.h"

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
#endif

struct App {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  TTF_Font* font = nullptr;
  Game* game = nullptr;
};

static App gApp;

static void shutdown_app() {
  // Game owns renderer after construction (your note), so don't destroy renderer here.
  if (gApp.game) {
    delete gApp.game;
    gApp.game = nullptr;
  }

  if (gApp.font) {
    TTF_CloseFont(gApp.font);
    gApp.font = nullptr;
  }

  if (gApp.window) {
    SDL_DestroyWindow(gApp.window);
    gApp.window = nullptr;
  }

  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

static bool init_app() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::printf("SDL_Init failed: %s\n", SDL_GetError());
    return false;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    std::printf("IMG_Init failed: %s\n", IMG_GetError());
    // Treat as fatal for web reliability
    shutdown_app();
    return false;
  }

  if (TTF_Init() != 0) {
    std::printf("TTF_Init failed: %s\n", TTF_GetError());
    shutdown_app();
    return false;
  }

  gApp.window = SDL_CreateWindow(
    "SDL2 Starter",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    960, 540,
    SDL_WINDOW_SHOWN
  );

  if (!gApp.window) {
    std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
    shutdown_app();
    return false;
  }

  gApp.renderer = SDL_CreateRenderer(
    gApp.window, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if (!gApp.renderer) {
    std::printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    shutdown_app();
    return false;
  }

  // IMPORTANT: in web builds, assets must be preloaded (CMake adds --preload-file).
  gApp.font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 28);
  if (!gApp.font) {
    std::printf("TTF_OpenFont failed: %s\n", TTF_GetError());
    shutdown_app();
    return false;
  }

  // IMPORTANT: Game may recreate renderer internally, so it owns renderer after this.
  gApp.game = new Game(gApp.window, gApp.renderer, gApp.font);
  return true;
}

#ifdef __EMSCRIPTEN__
static void em_frame() {
  // One frame per callback (non-blocking)
  gApp.game->tick();

  if (!gApp.game->isRunning()) {
    emscripten_cancel_main_loop();
    shutdown_app();
  }
}
#endif

int main(int, char**) {
  if (!init_app()) return 1;

#ifdef __EMSCRIPTEN__
  // Let browser drive the loop (60fps-ish; uses requestAnimationFrame)
  emscripten_set_main_loop(em_frame, 0, 1);
#else
  // Native can still run your blocking loop
  gApp.game->run();
  shutdown_app();
#endif

  return 0;
}
