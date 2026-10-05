#pragma once

#include "GameState.h"
#include "ModelCache.h"

class Renderer;

class WorldRenderer
{
  public:
    explicit WorldRenderer(Renderer &renderer);

    void BeginFrame(float time);
    void EndFrame(float time);
    void RenderWorld(const GameState &state);

  private:
    Vec2 ToScreen(Vec2 position) const;
    void DrawRect(Vec2 position, float width, float height, float rotation, float red, float green, float blue,
                  float alpha = 1.0f) const;
    void DrawShadow(Vec2 position, float width, float height) const;
    void DrawSprite(Vec2 position, int column, int row, float width, float height, bool shadow = true) const;
    void DrawHouse(Vec2 position, float roofRed, float roofGreen, float roofBlue) const;
    void DrawTree(Vec2 position, int variant) const;
    void DrawCampfire(Vec2 position) const;
    void DrawHerbPatch(Vec2 position) const;
    void DrawGround() const;

    Renderer &m_Renderer;
    mutable ModelCache m_ModelCache;
    const GameState *m_State = nullptr;
};
