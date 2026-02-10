// src/GameScene.h
#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

#include "Scene.h"
class Game;

enum class ObstacleType { JumpOver, DuckUnder };

struct Obstacle {
  SDL_FRect rect{};
  ObstacleType type{};
};

struct LevelDef {
  float length = 4000.0f;
  float bullSpeedBonus = 0.0f;
  int obstacleCount = 12;
  float obstacleSpacing = 260.0f;
};

class GameScene final : public Scene {
public:
  explicit GameScene(Game* game);
  ~GameScene() override; // NOT default: we must destroy textures

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* ren) override;
  void onRendererChanged(SDL_Renderer* newRenderer) override;

private:
  void buildLevels();
  void startLevel(int idx);
  void restartLevel();
  void generateObstacles(const LevelDef& def);

  void reloadTextures(SDL_Renderer* renderer);

  void applyInput();

  bool intersects(const SDL_FRect& a, const SDL_FRect& b) const;

  void checkCaught();
  void checkGoalReached();

private:
  Game* m_game = nullptr;

  // world tuning
  float groundY = 460.0f;
  float gravity = 2200.0f;
  float moveSpeed = 420.0f;
  float jumpVelocity = -900.0f;

  // chaser tuning
  float bullBaseSpeed = 260.0f;

  // camera / goal
  float camX = 0.0f;
  float goalX = 0.0f;

  // viewport + scaling state
  int viewportW = 960;
  int viewportH = 540;
  float screenGroundY = 460.0f;
  float zoomScale = 1.0f;
  float targetPlayerScreenRatio = 0.25f;
  float playerStandHeight = 92.0f;

  // levels
  std::vector<LevelDef> levels;
  int levelIndex = 0;
  bool waitingForEnter = false;

  // text strings (render via drawTextCentered)
  std::string hudLevelText;
  std::string overlayText;

  // player
  SDL_FRect player{};
  float vx = 0.0f;
  float vy = 0.0f;
  bool onGround = false;
  bool ducking = false;

  // chaser
  SDL_FRect bull{};
  float bullSpeed = 0.0f;

  // obstacles
  std::vector<Obstacle> obstacles;

  // textures (sprite sheets + static textures)
  SDL_Texture* texPlayerSheet = nullptr;
  SDL_Texture* texBullSheet   = nullptr;
  SDL_Texture* texBlock       = nullptr;
  SDL_Texture* texBar         = nullptr;
  SDL_Texture* texBg          = nullptr;

  // animation timers
  float playerAnimT = 0.0f;
  float bullAnimT   = 0.0f;

  // input (keyboard)
  bool leftHeld = false;
  bool rightHeld = false;
  bool duckHeld = false;
  bool jumpPressed = false; // one-shot

  // input (touch/mouse gestures) - additive, won't break keyboard
  bool gestureActive = false;
  bool gestureSwiped = false;

  bool touchRunHeld = false;
  bool touchDuckHeld = false;

  float gestureStartX = 0.0f;
  float gestureStartY = 0.0f;
  float gestureCurX   = 0.0f;
  float gestureCurY   = 0.0f;

  float swipeThresholdPx = 48.0f; // also clamped to 0.08*screenH

  void syncViewportMetrics();
  void refreshZoomFromViewport(int viewportW, int viewportH);
  SDL_FRect toScreenRect(const SDL_FRect& world) const;
};
