#pragma once

#include "GameState.h"
#include "ModelCache.h"
#include "LootTable.h"
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
    void DrawHouse(Vec2 position, float roofRed, float roofGreen, float roofBlue, int variant = 0) const;
    void DrawPixelNpc(const NpcState &npc, int variant) const;
    bool DrawRosterNpc(const NpcState &npc) const;
    bool DrawPixelAnimal(Vec2 position, int species, Vec2 facing, int frame,
                         float height, bool fallen = false) const;
    bool DrawChapter1Boar(const MonsterState &monster) const;
    bool DrawBoarDeath(Vec2 position, Vec2 facing, float height, float elapsed) const;
    bool DrawAnimalHit(Vec2 position, int species, Vec2 facing, float height, float remaining) const;
    void DrawLoot(Vec2 position, const LootTable::Drops &drops) const;
    void DrawQuestGlimmer(Vec2 position, float height = 112, float phase = 0) const;
    void DrawTree(Vec2 position, int variant) const;
    void DrawPixelPlayer(const GameState &state) const;
    void DrawGeneralPlayer(const GameState &state) const;
    void DrawPixelSprite(Vec2 position, unsigned int texture, float width, float height,
                         float pivotY, float sway = 0.0f) const;
    void DrawCampfire(Vec2 position) const;
    void DrawHerbPatch(Vec2 position) const;
    void DrawInvestigation(Vec2 position, int asset, bool disabled = false) const;
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
    unsigned int m_GeneralWalkTexture = 0, m_GeneralRunTexture = 0;
    unsigned int m_GeneralAttackTexture = 0;
    unsigned int m_SwordSlashTexture = 0;
    unsigned int m_ElderTexture = 0;
    unsigned int m_NoticeBoardTexture = 0;
    unsigned int m_RockTexture = 0;
    unsigned int m_PineTexture = 0;
    std::array<unsigned int, 2> m_NpcTextures = {}, m_HouseVariants = {};
    std::array<unsigned int, 20> m_RosterNpcTextures = {};
    unsigned int m_CampfireTexture = 0;
    unsigned int m_PlayerHitTexture = 0;
    unsigned int m_BoarLootTexture = 0, m_SearchTexture = 0;
    unsigned int m_AnimalCarcassTexture = 0;
    unsigned int m_BoarDeathTexture = 0;
    unsigned int m_QuestGlimmerTexture = 0;
    std::array<unsigned int, 3> m_AnimalHitTextures = {};
    unsigned int m_AncientGuardianTexture = 0;
    unsigned int m_ChatDeathTexture = 0;
    std::array<unsigned int, 3> m_ChatAttackTextures = {};
    std::array<std::array<unsigned int, 2>, 5> m_ChatHdTextures = {};
    std::array<unsigned int, 2> m_ChatImpactTextures = {};
    unsigned int m_PixelSceneryTexture = 0;
    std::array<unsigned int, 7> m_InvestigationTextures = {};
    std::array<unsigned int, 3> m_AttackHitTextures = {};
    std::array<unsigned int, 3> m_PixelAnimalTextures = {};
    std::array<unsigned int, 3> m_Chapter1BoarTextures = {};
    float m_TreePivotY = .17475277f;
    float m_HousePivotY = .30755001f;
    bool m_PixelPlayer = false;
    bool m_PixelTree = false;
    bool m_PixelTerrain = false;
    unsigned int m_TerrainTexture = 0, m_HumanTexture = 0;
    unsigned int m_BoarTexture = 0, m_DeerTexture = 0, m_SheepTexture = 0;
    const GameState *m_State = nullptr;
};
