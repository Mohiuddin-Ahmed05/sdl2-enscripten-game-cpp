// src/OptionsScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

class OptionsScene : public Scene {
public:
  explicit OptionsScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  Game* m_game = nullptr; // not owned

  // windowed resolutions to cycle
  int m_resIndex = 0;
  static constexpr int RES_COUNT = 5;
  const int m_resolutions[RES_COUNT][2] = {
    { 1024, 576 },  // Compact HD
    { 1280, 720 },  // Standard HD
    { 1600, 900 },  // Large HD
    { 1920, 1080 }, // Full HD
    { 2560, 1440 }  // QHD
  };
  const char* const m_resLabels[RES_COUNT] = {
    "Compact (1024 x 576)",
    "Standard (1280 x 720)",
    "Large (1600 x 900)",
    "Full HD (1920 x 1080)",
    "QHD (2560 x 1440)"
  };

  enum class PointerAction { None, Fullscreen, ResPrev, ResNext, Back };

  bool m_pointerDown = false;
  PointerAction m_pointerAction = PointerAction::None;

  void cycleResolution(int delta = 1);
  void applyResolutionAtIndex();
  void handlePointerEvent(const SDL_Event& e);
  PointerAction hitTestAction(float px, float py, int screenW, int screenH) const;
  void executeAction(PointerAction action);
};
