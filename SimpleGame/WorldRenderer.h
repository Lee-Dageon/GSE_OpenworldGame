#pragma once

#include "GameState.h"
#include "ModelCache.h"
#include <array>
#include <map>
#include <vector>

class Renderer;

class WorldRenderer
{
  public:
    explicit WorldRenderer(Renderer &renderer);
    ~WorldRenderer();
    WorldRenderer(const WorldRenderer &) = delete;
    WorldRenderer &operator=(const WorldRenderer &) = delete;

    void BeginFrame(float time);
    void EndFrame(float time);
    void RenderWorld(const GameState &state);

  private:
    struct GroundChunk
    {
        std::array<std::vector<float>, 24> batches;
    };
    struct GroundCache
    {
        std::map<std::pair<int, int>, GroundChunk> chunks;
        unsigned int buffer = 0;
        std::array<int, 24> first = {}, count = {};
        int minimumX = 0, maximumX = 0, minimumY = 0, maximumY = 0;
        Vec2 lake = {};
        float lakeRadius = -1;
        bool ready = false;
    };
    GroundChunk BuildGroundChunk(int chunkX, int chunkY) const;
    Vec2 ToScreen(Vec2 position) const;
    void DrawRect(Vec2 position, float width, float height, float rotation, float red, float green, float blue,
                  float alpha = 1.0f) const;
    void DrawShadow(Vec2 position, float width, float height, float castHeight = 0) const;
    void DrawSprite(Vec2 position, int column, int row, float width, float height, bool shadow = true) const;
    bool DrawAtlasSprite(Vec2 position, unsigned int texture, Vec2 facing, int frame,
                         float height, float pivotY, bool fallen = false) const;
    void DrawHouse(Vec2 position, float roofRed, float roofGreen, float roofBlue) const;
    void DrawTree(Vec2 position, int variant) const;
    void DrawCampfire(Vec2 position) const;
    void DrawHerbPatch(Vec2 position) const;
    void DrawGround() const;

    Renderer &m_Renderer;
    mutable ModelCache m_ModelCache;
    mutable GroundCache m_GroundCache;
    unsigned int m_TreeTexture = 0;
    unsigned int m_HouseTexture = 0;
    unsigned int m_PropsTexture = 0;
    unsigned int m_SceneryTexture = 0;
    unsigned int m_PlayerTexture = 0;
    unsigned int m_PlayerAttackTexture = 0;
    float m_TreePivotY = .17475277f;
    unsigned int m_TerrainTexture = 0, m_HumanTexture = 0;
    unsigned int m_BoarTexture = 0, m_DeerTexture = 0, m_SheepTexture = 0;
    const GameState *m_State = nullptr;
};
