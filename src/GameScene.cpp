// src/GameScene.cpp
// Adds mouse + touch gestures without breaking keyboard input:
//   * Swipe up    -> Jump
//   * Swipe down  -> Duck (held while finger stays down)
//   * Hold screen -> Run forward
// All gestures work with either touch or left mouse button.

#include "GameScene.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <string>
#include <cmath>

#include <SDL2/SDL_image.h>

#include "Game.h"
#include "Text.h" // drawTextCentered()

// ===================== YOUR SHEET LAYOUTS =====================
// bull_sheet.png: 2 rows x 4 columns (8 frames total)
static constexpr int BULL_COLS = 4;
static constexpr int BULL_ROWS = 2;
static constexpr float BULL_RUN_FPS = 10.0f;

// player_sheet.png:
// Row 0: 5 run frames
// Row 1: col0 jump, col1 duck
static constexpr int PLAYER_RUN_COLS = 5;
static constexpr int PLAYER_ROWS     = 2;
static constexpr int PLAYER_ROW_RUN  = 0;
static constexpr int PLAYER_ROW_MISC = 1;
static constexpr int PLAYER_COL_JUMP = 0;
static constexpr int PLAYER_COL_DUCK = 1;
static constexpr float PLAYER_RUN_FPS = 12.0f;
// =============================================================

static float frand01() { return (float)std::rand() / (float)RAND_MAX; }

static bool AABB(const SDL_FRect& a, const SDL_FRect& b) {
  return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
           a.y + a.h <= b.y || b.y + b.h <= a.y);
}

// Solid rules:
// - JumpOver blocks always.
// - DuckUnder blocks only when NOT ducking.
static bool isSolidForPlayer(const Obstacle& o, bool ducking) {
  if (o.type == ObstacleType::JumpOver) return true;
  if (o.type == ObstacleType::DuckUnder) return !ducking;
  return true;
}

static SDL_Texture* loadTexture(SDL_Renderer* r, const char* path) {
  if (!r || !path) return nullptr;
  SDL_Surface* surf = IMG_Load(path);
  if (!surf) {
    std::printf("IMG_Load failed (%s): %s\n", path, IMG_GetError());
    return nullptr;
  }
  SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
  SDL_FreeSurface(surf);
  if (!tex) std::printf("SDL_CreateTextureFromSurface failed (%s): %s\n", path, SDL_GetError());
  return tex;
}

static void destroyTex(SDL_Texture*& t) {
  if (t) { SDL_DestroyTexture(t); t = nullptr; }
}

// -----------------------------
// Touch + mouse helpers
// -----------------------------
static void getInputPosPixels(const SDL_Event& e, int rw, int rh, float& outX, float& outY) {
  if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP) {
    outX = e.tfinger.x * (float)rw; // tfinger is normalized 0..1
    outY = e.tfinger.y * (float)rh;
    return;
  }
  if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
    outX = (float)e.button.x;
    outY = (float)e.button.y;
    return;
  }
  if (e.type == SDL_MOUSEMOTION) {
    outX = (float)e.motion.x;
    outY = (float)e.motion.y;
    return;
  }
  outX = 0.0f; outY = 0.0f;
}

static void clearTouchHeld(bool& rightHeld, bool& duckHeld,
                           bool& touchRunHeld, bool& touchDuckHeld) {
  if (touchRunHeld)  { rightHeld = false; touchRunHeld = false; }
  if (touchDuckHeld) { duckHeld  = false; touchDuckHeld = false; }
}

GameScene::GameScene(Game* game) : m_game(game) {
  std::srand((unsigned)std::time(nullptr));
  buildLevels();

  // Player collider (no face)
  player.w = 44.0f;
  player.h = 92.0f;
  player.x = 120.0f;
  player.y = groundY - player.h;

  // Bull collider (no face)
  bull.w = 86.0f;
  bull.h = 62.0f;

  // ---- load textures ----
  // If these fail, game still runs (falls back to rectangles for that item)
  SDL_Renderer* r = (m_game ? m_game->renderer() : nullptr);
  reloadTextures(r);

  playerAnimT = 0.0f;
  bullAnimT   = 0.0f;

  startLevel(0);
}

GameScene::~GameScene() {
  destroyTex(texPlayerSheet);
  destroyTex(texBullSheet);
  destroyTex(texBlock);
  destroyTex(texBar);
  destroyTex(texBg);
}

void GameScene::buildLevels() {
  levels.clear();
  levels.reserve(10);

  for (int i = 0; i < 10; ++i) {
    LevelDef d;
    d.length = 3200.0f + i * 450.0f;
    d.bullSpeedBonus = i * 18.0f;
    d.obstacleCount = 10 + i * 2;
    d.obstacleSpacing = std::max(170.0f, 270.0f - i * 9.0f);
    levels.push_back(d);
  }
}

void GameScene::startLevel(int idx) {
  levelIndex = std::clamp(idx, 0, (int)levels.size() - 1);
  waitingForEnter = false;

  const LevelDef& def = levels[levelIndex];
  goalX = def.length;

  // reset player
  player.x = 120.0f;
  player.h = 92.0f;
  player.y = groundY - player.h;
  vx = 0.0f;
  vy = 0.0f;
  onGround = true;
  ducking = false;

  // reset bull behind player
  bull.x = player.x - 260.0f;
  bull.y = groundY - bull.h;
  bullSpeed = bullBaseSpeed + def.bullSpeedBonus;

  // camera
  camX = 0.0f;

  // obstacles
  generateObstacles(def);

  // HUD strings
  hudLevelText = "Level " + std::to_string(levelIndex + 1) + " / " + std::to_string((int)levels.size());
  overlayText.clear();

  // reset anim timers
  playerAnimT = 0.0f;
  bullAnimT   = 0.0f;

  // reset gesture/touch state (safe)
  gestureActive = false;
  gestureSwiped = false;
  clearTouchHeld(rightHeld, duckHeld, touchRunHeld, touchDuckHeld);
}

void GameScene::restartLevel() { startLevel(levelIndex); }

void GameScene::generateObstacles(const LevelDef& def) {
  obstacles.clear();
  obstacles.reserve(def.obstacleCount);

  float x = 520.0f;
  for (int i = 0; i < def.obstacleCount; ++i) {
    float jitter = (frand01() - 0.5f) * 120.0f;
    x += def.obstacleSpacing + jitter;

    Obstacle o;
    bool makeDuck = (frand01() < 0.45f);

    if (makeDuck) {
      // overhead bar
      o.type = ObstacleType::DuckUnder;
      o.rect.w = 140.0f;
      o.rect.h = 24.0f;
      o.rect.x = x;
      o.rect.y = (groundY - 92.0f) + 22.0f;
    } else {
      // ground block (solid)
      o.type = ObstacleType::JumpOver;
      o.rect.w = 58.0f;
      o.rect.h = 48.0f;
      o.rect.x = x;
      o.rect.y = groundY - o.rect.h;
    }

    if (o.rect.x < def.length - 220.0f) obstacles.push_back(o);
  }
}

void GameScene::handleEvent(const SDL_Event& e) {
  // -------- keyboard input (unchanged) --------
  if (e.type == SDL_KEYDOWN && !e.key.repeat) {
    switch (e.key.keysym.sym) {
      case SDLK_LEFT:  leftHeld = true; break;
      case SDLK_a:     leftHeld = true; break;

      case SDLK_RIGHT: rightHeld = true; break;
      case SDLK_d:     rightHeld = true; break;

      case SDLK_DOWN:  duckHeld = true; break;
      case SDLK_s:     duckHeld = true; break;

      case SDLK_UP:    jumpPressed = true; break;
      case SDLK_w:     jumpPressed = true; break;
      case SDLK_SPACE: jumpPressed = true; break;

      case SDLK_RETURN:
        if (waitingForEnter) {
          int next = levelIndex + 1;
          if (next >= (int)levels.size()) startLevel(0);
          else startLevel(next);
        }
        break;

      default: break;
    }
  }

  if (e.type == SDL_KEYUP) {
    switch (e.key.keysym.sym) {
      case SDLK_LEFT:  leftHeld = false; break;
      case SDLK_a:     leftHeld = false; break;

      case SDLK_RIGHT: rightHeld = false; break;
      case SDLK_d:     rightHeld = false; break;

      case SDLK_DOWN:  duckHeld = false; break;
      case SDLK_s:     duckHeld = false; break;

      default: break;
    }
  }

  // ------------------------------------------
  // Touch + Mouse gestures (additive, safe)
  // ------------------------------------------
  const bool isPointerDown =
    (e.type == SDL_FINGERDOWN) ||
    (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT);

  const bool isPointerUp =
    (e.type == SDL_FINGERUP) ||
    (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT);

  const bool isPointerMove =
    (e.type == SDL_FINGERMOTION) ||
    (e.type == SDL_MOUSEMOTION);

  if (isPointerDown) {
    int rw = 960, rh = 540;
    if (m_game) m_game->getRenderSize(rw, rh);
    if (rw > 0 && rh > 0) { viewportW = rw; viewportH = rh; }

    getInputPosPixels(e, rw, rh, gestureStartX, gestureStartY);
    gestureCurX = gestureStartX;
    gestureCurY = gestureStartY;

    gestureActive = true;
    gestureSwiped = false;
    clearTouchHeld(rightHeld, duckHeld, touchRunHeld, touchDuckHeld);

    // Holding anywhere keeps the player running forward.
    rightHeld = true;
    touchRunHeld = true;
  }

  if (gestureActive && isPointerMove) {
    int rw = 960, rh = 540;
    if (m_game) m_game->getRenderSize(rw, rh);
    if (rw > 0 && rh > 0) { viewportW = rw; viewportH = rh; }

    getInputPosPixels(e, rw, rh, gestureCurX, gestureCurY);

    const float dy = gestureCurY - gestureStartY;
    const float absDy = std::fabs(dy);

    const float thresh = std::max(swipeThresholdPx, 0.08f * (float)rh);

    if (!gestureSwiped && absDy >= thresh) {
      gestureSwiped = true;

      // Swipe Up => Jump
      if (dy < 0.0f) {
        jumpPressed = true;
      }
      // Swipe Down => Duck while held
      else {
        duckHeld = true;
        touchDuckHeld = true;
      }
    }
  }

  if (gestureActive && isPointerUp) {
    gestureActive = false;
    gestureSwiped = false;

    // Release what touch/mouse held (won't affect keyboard-held keys)
    clearTouchHeld(rightHeld, duckHeld, touchRunHeld, touchDuckHeld);
  }
}

void GameScene::applyInput() {
  // horizontal
  vx = 0.0f;
  if (leftHeld)  vx -= moveSpeed;
  if (rightHeld) vx += moveSpeed;

  // duck (only grounded)
  if (duckHeld && onGround) {
    if (!ducking) {
      ducking = true;
      float oldH = player.h;
      player.h = 56.0f;
      player.y += (oldH - player.h); // keep feet grounded
    }
  } else {
    if (ducking) {
      // stand up only if not colliding when standing
      float oldH = player.h;
      float newH = 92.0f;

      SDL_FRect test = player;
      test.y -= (newH - oldH);
      test.h = newH;

      bool blocked = false;
      for (const auto& o : obstacles) {
        if (!isSolidForPlayer(o, /*ducking=*/false)) continue;
        if (AABB(test, o.rect)) { blocked = true; break; }
      }

      if (!blocked) {
        ducking = false;
        player.h = newH;
        player.y = test.y;
      }
    }
  }

  // jump
  if (jumpPressed && onGround && !ducking) {
    vy = jumpVelocity;
    onGround = false;
  }
}

void GameScene::update(float dt) {
  syncViewportMetrics();

  if (waitingForEnter) {
    jumpPressed = false;
    return;
  }

  applyInput();

  // advance animations
  playerAnimT += dt;
  bullAnimT   += dt;

  // gravity
  vy += gravity * dt;

  // Previous vertical state for crossing checks
  const float prevY = player.y;
  const float prevBottom = prevY + player.h;

  // 1) Move X, resolve X collisions
  player.x += vx * dt;
  if (player.x < 30.0f) player.x = 30.0f;

  if (vx != 0.0f) {
    for (const auto& o : obstacles) {
      if (!isSolidForPlayer(o, ducking)) continue;
      if (!AABB(player, o.rect)) continue;

      if (vx > 0.0f) player.x = o.rect.x - player.w;
      else           player.x = o.rect.x + o.rect.w;
    }
  }

  // 2) Move Y, resolve Y collisions
  player.y += vy * dt;
  onGround = false;

  for (const auto& o : obstacles) {
    if (!isSolidForPlayer(o, ducking)) continue;
    if (!AABB(player, o.rect)) continue;

    const float oTop = o.rect.y;
    const float oBottom = o.rect.y + o.rect.h;

    const float pTop = player.y;
    const float pBottom = player.y + player.h;

    if (vy > 0.0f) {
      // falling -> land on top
      if (prevBottom <= oTop + 0.5f && pBottom >= oTop) {
        player.y = oTop - player.h;
        vy = 0.0f;
        onGround = true;
      }
    } else if (vy < 0.0f) {
      // rising -> bonk underside
      if (prevY >= oBottom - 0.5f && pTop <= oBottom) {
        player.y = oBottom;
        vy = 0.0f;
      }
    } else {
      // vy == 0, overlapping: prefer standing if we were above
      if (prevBottom <= oTop + 2.0f) {
        player.y = oTop - player.h;
        onGround = true;
      } else {
        player.y = oBottom;
      }
    }
  }

  // 3) World ground
  float floorY = groundY - player.h;
  if (player.y >= floorY) {
    player.y = floorY;
    vy = 0.0f;
    onGround = true;
  }

  // bull chase
  bull.x += bullSpeed * dt;
  bull.y = groundY - bull.h;

  // camera follow
  const float viewportWorldWidth = (float)viewportW / std::max(0.01f, zoomScale);
  float targetCam = player.x - viewportWorldWidth * 0.30f;
  const float maxCam = std::max(0.0f, goalX - viewportWorldWidth);
  camX = std::clamp(targetCam, 0.0f, maxCam);

  // conditions
  checkCaught();
  checkGoalReached();

  jumpPressed = false;
}

bool GameScene::intersects(const SDL_FRect& a, const SDL_FRect& b) const {
  return AABB(a, b);
}

void GameScene::checkCaught() {
  if (intersects(bull, player) || (bull.x + bull.w) >= (player.x + 8.0f)) {
    restartLevel();
  }
}

void GameScene::checkGoalReached() {
  if (player.x >= goalX && !waitingForEnter) {
    waitingForEnter = true;

    const int nextHuman = levelIndex + 2;
    const bool hasNext = nextHuman <= (int)levels.size();

    overlayText = hasNext
      ? ("Press ENTER to begin Level " + std::to_string(nextHuman))
      : "Press ENTER to restart";
  }
}

void GameScene::render(SDL_Renderer* ren) {
  syncViewportMetrics();
  int rw = viewportW;
  int rh = viewportH;

  // background
  SDL_SetRenderDrawColor(ren, 10, 12, 16, 255);
  SDL_RenderClear(ren);

  if (texBg) {
    int bgW = 0, bgH = 0;
    SDL_QueryTexture(texBg, nullptr, nullptr, &bgW, &bgH);
    if (bgW > 0 && bgH > 0) {
      float scale = (float)rw / (float)bgW;
      float destH = (float)bgH * scale;
      SDL_FRect dest { 0.0f, (float)rh - destH, (float)rw, destH };
      SDL_RenderCopyF(ren, texBg, nullptr, &dest);
    }
  }

  // ground band
  SDL_SetRenderDrawColor(ren, 40, 45, 55, 255);
  SDL_FRect ground { 0.0f, screenGroundY, (float)rw, (float)rh - screenGroundY };
  if (ground.y < 0.0f) {
    ground.h += ground.y;
    ground.y = 0.0f;
  }
  if (ground.h < 0.0f) ground.h = 0.0f;
  SDL_RenderFillRectF(ren, &ground);

  // goal marker
  SDL_SetRenderDrawColor(ren, 190, 200, 220, 255);
  SDL_FRect goalRect = toScreenRect(SDL_FRect{ goalX, groundY - 160.0f, 16.0f, 160.0f });
  SDL_RenderFillRectF(ren, &goalRect);

  // obstacles
  for (const auto& o : obstacles) {
    SDL_FRect rf = toScreenRect(o.rect);

    if (o.type == ObstacleType::JumpOver) {
      if (texBlock) SDL_RenderCopyF(ren, texBlock, nullptr, &rf);
      else {
        SDL_SetRenderDrawColor(ren, 90, 180, 120, 255);
        SDL_RenderFillRectF(ren, &rf);
      }
    } else {
      if (texBar) SDL_RenderCopyF(ren, texBar, nullptr, &rf);
      else {
        SDL_SetRenderDrawColor(ren, 90, 140, 200, 255);
        SDL_RenderFillRectF(ren, &rf);
      }
    }
  }

  // bull (2 rows x 4 cols)
  {
    SDL_FRect bf = toScreenRect(bull);

    if (texBullSheet) {
      int texW = 0, texH = 0;
      SDL_QueryTexture(texBullSheet, nullptr, nullptr, &texW, &texH);

      if (texW > 0 && texH > 0) {
        const int frameW = texW / BULL_COLS;
        const int frameH = texH / BULL_ROWS;

        const int totalFrames = BULL_COLS * BULL_ROWS;
        int f = (int)(bullAnimT * BULL_RUN_FPS) % std::max(1, totalFrames);
        int row = f / BULL_COLS;
        int col = f % BULL_COLS;

        SDL_Rect src { col * frameW, row * frameH, frameW, frameH };
        SDL_RenderCopyF(ren, texBullSheet, &src, &bf);
      } else {
        SDL_SetRenderDrawColor(ren, 210, 70, 70, 255);
        SDL_RenderFillRectF(ren, &bf);
      }
    } else {
      SDL_SetRenderDrawColor(ren, 210, 70, 70, 255);
      SDL_RenderFillRectF(ren, &bf);
    }
  }

  // player (row0 = 5 run frames, row1 col0=jump col1=duck)
  {
    SDL_FRect pf = toScreenRect(player);

    if (texPlayerSheet) {
      int texW = 0, texH = 0;
      SDL_QueryTexture(texPlayerSheet, nullptr, nullptr, &texW, &texH);

      if (texW > 0 && texH > 0) {
        const int frameW = texW / PLAYER_RUN_COLS;
        const int frameH = texH / PLAYER_ROWS;

        SDL_Rect src{};

        if (!onGround) {
          // jump = row1 col0
          src = { PLAYER_COL_JUMP * frameW, PLAYER_ROW_MISC * frameH, frameW, frameH };
        } else if (ducking) {
          // duck = row1 col1
          src = { PLAYER_COL_DUCK * frameW, PLAYER_ROW_MISC * frameH, frameW, frameH };
        } else {
          // run = row0 col0..4
          int frame = 0;
          if (std::abs(vx) > 1.0f) {
            frame = (int)(playerAnimT * PLAYER_RUN_FPS) % PLAYER_RUN_COLS;
          }
          src = { frame * frameW, PLAYER_ROW_RUN * frameH, frameW, frameH };
        }

        bool flip = (vx < 0.0f);
        SDL_RenderCopyExF(ren, texPlayerSheet, &src, &pf, 0.0, nullptr,
                          flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
      } else {
        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderFillRectF(ren, &pf);
      }
    } else {
      SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
      SDL_RenderFillRectF(ren, &pf);
    }
  }

  // progress bar
  float t = std::clamp(player.x / std::max(1.0f, goalX), 0.0f, 1.0f);
  SDL_SetRenderDrawColor(ren, 120, 160, 240, 255);
  SDL_FRect bar { 20.0f, 20.0f, (rw - 40.0f) * t, 10.0f };
  SDL_RenderFillRectF(ren, &bar);

  // HUD: current level label
  if (m_game && m_game->font()) {
    SDL_FRect hudBox { 20.0f, 44.0f, 260.0f, 34.0f };
    drawTextCentered(ren, m_game->font(), hudLevelText.c_str(), hudBox);
  }

  // overlay when waiting for Enter
  if (waitingForEnter) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 140);
    SDL_FRect overlay { 0.0f, 0.0f, (float)rw, (float)rh };
    SDL_RenderFillRectF(ren, &overlay);

    SDL_SetRenderDrawColor(ren, 240, 240, 240, 220);
    SDL_FRect panel { rw * 0.20f, rh * 0.35f, rw * 0.60f, rh * 0.30f };
    SDL_RenderFillRectF(ren, &panel);

    if (m_game && m_game->font() && !overlayText.empty()) {
      SDL_FRect msgBox { panel.x, panel.y, panel.w, panel.h };
      drawTextCentered(ren, m_game->font(), overlayText.c_str(), msgBox);
    }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
  }
}

void GameScene::reloadTextures(SDL_Renderer* r) {
  destroyTex(texPlayerSheet);
  destroyTex(texBullSheet);
  destroyTex(texBlock);
  destroyTex(texBar);
  destroyTex(texBg);

  if (!r) return;

  texPlayerSheet = loadTexture(r, "assets/sprites/player_sheet.png");
  texBullSheet   = loadTexture(r, "assets/sprites/bull_sheet.png");
  texBlock       = loadTexture(r, "assets/sprites/block.png");
  texBar         = loadTexture(r, "assets/sprites/bar.png");
  texBg          = loadTexture(r, "assets/sprites/bg.png");
}

void GameScene::onRendererChanged(SDL_Renderer* newRenderer) {
  reloadTextures(newRenderer);
}

void GameScene::syncViewportMetrics() {
  int w = viewportW;
  int h = viewportH;
  if (m_game) m_game->getRenderSize(w, h);
  if (w <= 0) w = 960;
  if (h <= 0) h = 540;
  viewportW = w;
  viewportH = h;
  refreshZoomFromViewport(viewportW, viewportH);
}

void GameScene::refreshZoomFromViewport(int viewportW, int viewportH) {
  const float vh = (float)std::max(1, viewportH);
  const float desiredScreenHeight = vh * targetPlayerScreenRatio;
  const float baseHeight = std::max(1.0f, playerStandHeight);
  const float computedZoom = desiredScreenHeight / baseHeight;
  zoomScale = std::clamp(computedZoom, 0.5f, 3.5f);

  const float padding = std::max(36.0f, vh * 0.08f);
  const float grounded = vh - padding;
  screenGroundY = std::clamp(grounded, 0.0f, vh);
}

SDL_FRect GameScene::toScreenRect(const SDL_FRect& world) const {
  SDL_FRect out{};
  out.x = (world.x - camX) * zoomScale;
  out.w = world.w * zoomScale;
  out.h = world.h * zoomScale;
  out.y = screenGroundY + (world.y - groundY) * zoomScale;
  return out;
}
