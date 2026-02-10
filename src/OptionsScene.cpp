// src/OptionsScene.cpp
#include "OptionsScene.h"

#include <SDL2/SDL.h>
#include <cstdio>

#include "Text.h"

namespace {

bool isPointerDownEvent(const SDL_Event& e) {
  return e.type == SDL_FINGERDOWN ||
         (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT);
}

bool isPointerUpEvent(const SDL_Event& e) {
  return e.type == SDL_FINGERUP ||
         (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT);
}

bool extractPointerPosition(const SDL_Event& e, int rw, int rh, float& outX, float& outY) {
  if (rw <= 0) rw = 1;
  if (rh <= 0) rh = 1;

  switch (e.type) {
    case SDL_FINGERDOWN:
    case SDL_FINGERMOTION:
    case SDL_FINGERUP:
      outX = e.tfinger.x * (float)rw;
      outY = e.tfinger.y * (float)rh;
      return true;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      outX = (float)e.button.x;
      outY = (float)e.button.y;
      return true;
    case SDL_MOUSEMOTION:
      outX = (float)e.motion.x;
      outY = (float)e.motion.y;
      return true;
    default:
      break;
  }
  return false;
}

bool contains(const SDL_FRect& r, float px, float py) {
  return (px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h);
}

struct OptionsLayout {
  SDL_FRect fullscreenRect{};
  SDL_FRect resPrevRect{};
  SDL_FRect resNextRect{};
  SDL_FRect backRect{};
};

OptionsLayout buildLayout(int screenW, int screenH) {
  if (screenW <= 0) screenW = 1;
  if (screenH <= 0) screenH = 1;
  OptionsLayout layout;
  float panelX = screenW * 0.5f - 260.f;
  float panelY = screenH * 0.5f - 170.f;
  float panelW = 520.f;

  SDL_FRect modeBox { panelX, panelY + 86.f, panelW, 36.f };
  layout.fullscreenRect = SDL_FRect{ modeBox.x, modeBox.y - 6.f, modeBox.w, modeBox.h + 12.f };

  SDL_FRect resBox { panelX, panelY + 132.f, panelW, 36.f };
  SDL_FRect presetBox { panelX, panelY + 176.f, panelW, 32.f };
  SDL_FRect resArea { resBox.x, resBox.y - 6.f, resBox.w, (presetBox.y + presetBox.h + 6.f) - (resBox.y - 6.f) };
  layout.resPrevRect = resArea;
  layout.resPrevRect.w = resArea.w * 0.5f;
  layout.resNextRect = layout.resPrevRect;
  layout.resNextRect.x += layout.resPrevRect.w;

  SDL_FRect hintBox { panelX, panelY + 236.f, panelW, 80.f };
  layout.backRect = hintBox;

  return layout;
}

} // namespace

OptionsScene::OptionsScene(Game* game) : m_game(game) {}

void OptionsScene::cycleResolution(int delta) {
  if (!m_game) return;

  m_resIndex = (m_resIndex + RES_COUNT + delta) % RES_COUNT;
  applyResolutionAtIndex();
}

void OptionsScene::applyResolutionAtIndex() {
  if (!m_game) return;
  int w = m_resolutions[m_resIndex][0];
  int h = m_resolutions[m_resIndex][1];
  m_game->setWindowedResolution(w, h);
}

void OptionsScene::handleEvent(const SDL_Event& e) {
  if (!m_game) return;

  if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
    switch (e.key.keysym.sym) {
      case SDLK_f: // fullscreen toggle
        m_game->toggleFullscreen();
        break;

      case SDLK_r: // cycle resolution (windowed only)
      case SDLK_RIGHT:
        cycleResolution(1);
        break;

      case SDLK_LEFT:
        cycleResolution(-1);
        break;

      case SDLK_ESCAPE:
        m_game->requestScene(Game::SceneId::Menu);
        break;

      default:
        break;
    }
  }

  handlePointerEvent(e);
}

void OptionsScene::update(float) {
  // nothing yet
}

void OptionsScene::render(SDL_Renderer* r) {
  if (!m_game || !r) return;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);

  SDL_SetRenderDrawColor(r, 16, 12, 20, 255);
  SDL_RenderClear(r);

  // centered panel
  SDL_FRect panel { w * 0.5f - 260.f, h * 0.5f - 170.f, 520.f, 340.f };
  SDL_SetRenderDrawColor(r, 30, 34, 48, 255);
  SDL_RenderFillRectF(r, &panel);
  SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
  SDL_RenderDrawRectF(r, &panel);

  // Title
  SDL_FRect titleBox { panel.x, panel.y + 18.f, panel.w, 44.f };
  drawTextCentered(r, m_game->font(), "Options", titleBox);

  // Current mode line
  const char* mode = m_game->isFullscreen() ? "Fullscreen: ON (F to toggle)" : "Fullscreen: OFF (F to toggle)";
  SDL_FRect modeBox { panel.x, panel.y + 86.f, panel.w, 36.f };
  drawTextCentered(r, m_game->font(), mode, modeBox);

  // Resolution line (renderer output size reflects actual size)
  char buf[128];
  std::snprintf(buf, sizeof(buf), "Resolution: %dx%d (R or arrow keys)", w, h);
  SDL_FRect resBox { panel.x, panel.y + 132.f, panel.w, 36.f };
  drawTextCentered(r, m_game->font(), buf, resBox);

  // Preset label for clarity
  SDL_FRect presetBox { panel.x, panel.y + 176.f, panel.w, 32.f };
  drawTextCentered(r, m_game->font(), m_resLabels[m_resIndex], presetBox);

  // Hint
  SDL_FRect hintBox { panel.x, panel.y + 236.f, panel.w, 80.f };
  drawTextCentered(r, m_game->font(), "Click/tap rows or press ESC to return", hintBox);
}

void OptionsScene::handlePointerEvent(const SDL_Event& e) {
  const bool pointerDown = isPointerDownEvent(e);
  const bool pointerUp = isPointerUpEvent(e);
  if (!pointerDown && !pointerUp) return;

  int rw = 0, rh = 0;
  m_game->getRenderSize(rw, rh);
  float px = 0.0f, py = 0.0f;
  if (!extractPointerPosition(e, rw, rh, px, py)) return;

  if (pointerDown) {
    PointerAction action = hitTestAction(px, py, rw, rh);
    if (action != PointerAction::None) {
      m_pointerDown = true;
      m_pointerAction = action;
    } else {
      m_pointerDown = false;
      m_pointerAction = PointerAction::None;
    }
  }

  if (pointerUp) {
    PointerAction action = hitTestAction(px, py, rw, rh);
    if (m_pointerDown && action == m_pointerAction) {
      executeAction(action);
    }
    m_pointerDown = false;
    m_pointerAction = PointerAction::None;
  }
}

OptionsScene::PointerAction OptionsScene::hitTestAction(float px, float py, int screenW, int screenH) const {
  OptionsLayout layout = buildLayout(screenW, screenH);

  if (contains(layout.fullscreenRect, px, py)) return PointerAction::Fullscreen;
  if (contains(layout.resPrevRect, px, py)) return PointerAction::ResPrev;
  if (contains(layout.resNextRect, px, py)) return PointerAction::ResNext;
  if (contains(layout.backRect, px, py)) return PointerAction::Back;
  return PointerAction::None;
}

void OptionsScene::executeAction(PointerAction action) {
  switch (action) {
    case PointerAction::Fullscreen:
      m_game->toggleFullscreen();
      break;
    case PointerAction::ResPrev:
      cycleResolution(-1);
      break;
    case PointerAction::ResNext:
      cycleResolution(1);
      break;
    case PointerAction::Back:
      m_game->requestScene(Game::SceneId::Menu);
      break;
    case PointerAction::None:
    default:
      break;
  }
}
