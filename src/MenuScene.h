// src/MenuScene.h
#pragma once

#include <SDL2/SDL.h>

#include "Scene.h"

class Game;

// Minimal menu scene: Start / Options / Quit
class MenuScene : public Scene {
public:
  explicit MenuScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  Game* m_game = nullptr; // not owned
  int   m_index = 0;
  bool  m_pointerDown = false;
  int   m_pointerIndex = -1;

  float pulse() const; // simple highlight animation
  void activateSelection(int idx);
  int  hitTestItem(float px, float py, int screenW, int screenH) const;
};
