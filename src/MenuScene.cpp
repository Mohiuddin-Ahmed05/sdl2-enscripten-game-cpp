// src/MenuScene.cpp
#include "MenuScene.h"

#include <SDL2/SDL.h>
#include <algorithm>

#include "Game.h"
#include "Text.h" // drawTextCentered(...)

namespace {

bool isPointerDownEvent(const SDL_Event& e) {
  return e.type == SDL_FINGERDOWN ||
         (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT);
}

bool isPointerUpEvent(const SDL_Event& e) {
  return e.type == SDL_FINGERUP ||
         (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT);
}

bool isPointerMotionEvent(const SDL_Event& e) {
  return e.type == SDL_FINGERMOTION || e.type == SDL_MOUSEMOTION;
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

} // namespace

struct MenuItem { const char* id; };

static constexpr MenuItem MENU[] = {
  {"Start"},
  {"Options"},
  {"Quit"}
};
static constexpr int MENU_COUNT = (int)(sizeof(MENU) / sizeof(MENU[0]));

MenuScene::MenuScene(Game* game) : m_game(game) {}

float MenuScene::pulse() const {
  // 0..1 pulse
  return 0.5f + 0.5f * SDL_sinf((float)SDL_GetTicks() * 0.008f);
}

void MenuScene::handleEvent(const SDL_Event& e) {
  if (!m_game) return;

  if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
    switch (e.key.keysym.sym) {
      case SDLK_UP:
      case SDLK_w:
        m_index = (m_index - 1 + MENU_COUNT) % MENU_COUNT;
        break;

      case SDLK_DOWN:
      case SDLK_s:
        m_index = (m_index + 1) % MENU_COUNT;
        break;

      case SDLK_RETURN:
      case SDLK_KP_ENTER:
        activateSelection(m_index);
        break;

      default:
        break;
    }
  }

  const bool pointerDown = isPointerDownEvent(e);
  const bool pointerUp = isPointerUpEvent(e);
  const bool pointerMotion = isPointerMotionEvent(e);

  if (pointerDown || pointerUp || pointerMotion) {
    int rw = 0, rh = 0;
    m_game->getRenderSize(rw, rh);

    float px = 0.0f, py = 0.0f;
    if (!extractPointerPosition(e, rw, rh, px, py)) return;

    if (pointerDown) {
      int hit = hitTestItem(px, py, rw, rh);
      if (hit >= 0) {
        m_index = hit;
        m_pointerDown = true;
        m_pointerIndex = hit;
      } else {
        m_pointerDown = false;
        m_pointerIndex = -1;
      }
    }

    if (pointerMotion) {
      int hit = hitTestItem(px, py, rw, rh);
      if (hit >= 0) {
        m_index = hit;
        if (m_pointerDown) m_pointerIndex = hit;
      }
    }

    if (pointerUp) {
      int hit = hitTestItem(px, py, rw, rh);
      if (m_pointerDown && hit >= 0 && hit == m_pointerIndex) {
        activateSelection(hit);
      }
      m_pointerDown = false;
      m_pointerIndex = -1;
    }
  }
}

void MenuScene::update(float) {
  // nothing yet (pulse is time-based via SDL_GetTicks)
}

void MenuScene::render(SDL_Renderer* r) {
  if (!m_game || !r) return;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);

  SDL_SetRenderDrawColor(r, 12, 12, 16, 255);
  SDL_RenderClear(r);

  const float p = pulse();

  const float bw = 320.f, bh = 70.f;
  const float gap = 18.f;
  const float totalH = MENU_COUNT * bh + (MENU_COUNT - 1) * gap;

  float x = (w - bw) * 0.5f;
  float y = (h - totalH) * 0.5f;

  for (int i = 0; i < MENU_COUNT; i++) {
    SDL_FRect box { x, y + i * (bh + gap), bw, bh };

    // background fill
    if (i == m_index) {
      Uint8 bright = (Uint8)(140 + 60 * p);
      SDL_SetRenderDrawColor(r, 80, bright, 255, 255);
    } else {
      SDL_SetRenderDrawColor(r, 30, 34, 48, 255);
    }
    SDL_RenderFillRectF(r, &box);

    // outline
    SDL_SetRenderDrawColor(r, 50, 60, 80, 255);
    SDL_RenderDrawRectF(r, &box);

    // inner outline for depth
    SDL_FRect inner { box.x + 4, box.y + 4, box.w - 8, box.h - 8 };
    SDL_SetRenderDrawColor(r, 12, 12, 16, 140);
    SDL_RenderDrawRectF(r, &inner);

    // text label
    drawTextCentered(r, m_game->font(), MENU[i].id, box);

    // tiny indicator on selected item
    if (i == m_index) {
      SDL_FRect notch { box.x + 10, box.y + box.h * 0.5f - 6, 12, 12 };
      SDL_SetRenderDrawColor(r, 12, 12, 16, 220);
      SDL_RenderFillRectF(r, &notch);
    }
  }
}

void MenuScene::activateSelection(int idx) {
  if (!m_game) return;
  if (idx < 0 || idx >= MENU_COUNT) return;

  if (idx == 0)      m_game->requestScene(Game::SceneId::Play);
  else if (idx == 1) m_game->requestScene(Game::SceneId::Options);
  else if (idx == 2) m_game->requestQuit();
}

int MenuScene::hitTestItem(float px, float py, int screenW, int screenH) const {
  if (screenW <= 0) screenW = 1;
  if (screenH <= 0) screenH = 1;
  const float bw = 320.f, bh = 70.f;
  const float gap = 18.f;
  const float totalH = MENU_COUNT * bh + (MENU_COUNT - 1) * gap;

  float x = (screenW - bw) * 0.5f;
  float y = (screenH - totalH) * 0.5f;

  for (int i = 0; i < MENU_COUNT; ++i) {
    SDL_FRect box { x, y + i * (bh + gap), bw, bh };
    if (contains(box, px, py)) return i;
  }
  return -1;
}
