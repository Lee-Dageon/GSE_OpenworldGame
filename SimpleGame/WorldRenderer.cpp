#include "stdafx.h"
#include "WorldRenderer.h"

#include "Dependencies\\glew.h"
#include "Renderer.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr float Pi = 3.14159265f;
constexpr float TileWidth = 64.0f;
constexpr float TileHeight = 32.0f;

float Distance(Vec2 first, Vec2 second)
{
    const float x = first.x - second.x;
    const float z = first.z - second.z;
    return std::sqrt(x * x + z * z);
}

float DistanceToSegment(Vec2 position, Vec2 start, Vec2 end)
{
    const float dx = end.x - start.x;
    const float dz = end.z - start.z;
    const float lengthSquared = dx * dx + dz * dz;
    const float progress = std::max(0.0f, std::min(1.0f, ((position.x - start.x) * dx + (position.z - start.z) * dz) /
                                                        lengthSquared));
    return Distance(position, {start.x + dx * progress, start.z + dz * progress});
}

float RoadDistance(Vec2 position)
{
    float distance = DistanceToSegment(position, {0, -34}, {0, 34});
    distance = std::min(distance, DistanceToSegment(position, {-38, 0}, {38, 0}));
    distance = std::min(distance, DistanceToSegment(position, {-30, -26}, {30, 26}));
    distance = std::min(distance, DistanceToSegment(position, {-28, 25}, {28, -24}));
    return std::min(distance, DistanceToSegment(position, {0, 28}, {36, 34}));
}
}

WorldRenderer::WorldRenderer(Renderer &renderer) : m_Renderer(renderer) {}

void WorldRenderer::BeginFrame(float time)
{
    m_Renderer.BeginFrame(time);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void WorldRenderer::EndFrame(float time)
{
    m_Renderer.EndFrame(time);
}

Vec2 WorldRenderer::ToScreen(Vec2 position) const
{
    const float x = position.x - m_State->player.x;
    const float z = position.z - m_State->player.z;
    return {(x - z) * TileWidth * .5f, (x + z) * TileHeight * .5f - 70.0f};
}

void WorldRenderer::DrawRect(Vec2 position, float width, float height, float rotation, float red, float green,
                             float blue, float alpha) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawSolidRect(screen.x, screen.z, width, height, rotation, red, green, blue, alpha);
}

void WorldRenderer::DrawShadow(Vec2 position, float width, float height) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawSolidRect(screen.x + 9, screen.z - 13, width, height, -.12f, .02f, .03f, .02f, .22f);
}

void WorldRenderer::DrawSprite(Vec2 position, int column, int row, float, float, bool shadow) const
{
    const Vec2 screen = ToScreen(position);
    if (shadow)
        DrawShadow(position, row >= 2 ? 54.0f : 42.0f, 10.0f);
    if (row >= 2)
    {
        const float furRed = column == 0 ? .55f : (column == 1 ? .27f : .64f);
        const float furGreen = column == 0 ? .34f : (column == 1 ? .16f : .42f);
        const float furBlue = column == 2 ? .22f : .09f;
        m_Renderer.DrawSolidRect(screen.x, screen.z + 9, 44, 23, 0, furRed, furGreen, furBlue, 1);
        m_Renderer.DrawSolidRect(screen.x + 24, screen.z + 16, 20, 18, 0, furRed * .85f, furGreen * .85f, furBlue, 1);
        m_Renderer.DrawSolidRect(screen.x + 32, screen.z + 28, 7, 11, .28f, .88f, .75f, .46f, 1);
        for (int leg = -1; leg <= 1; leg += 2)
            m_Renderer.DrawSolidRect(screen.x + leg * 14, screen.z - 5, 6, 18, 0, .16f, .09f, .04f, 1);
        return;
    }

    const float colors[4][3] = {{.14f, .38f, .24f}, {.55f, .22f, .12f}, {.16f, .34f, .58f}, {.48f, .18f, .40f}};
    const float *cloth = colors[column % 4];
    const float bob = std::sin(m_State->time * 5.0f + static_cast<float>(column)) * 1.5f;
    m_Renderer.DrawSolidRect(screen.x, screen.z + 18 + bob, 24, 34, 0, cloth[0], cloth[1], cloth[2], 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 42 + bob, 17, 17, 0, .88f, .67f, .45f, 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 56 + bob, 27, 8, 0, .13f, .08f, .04f, 1);
    m_Renderer.DrawSolidRect(screen.x - 9, screen.z + 19 + bob, 7, 25, -.20f, cloth[0] * .72f, cloth[1] * .72f,
                             cloth[2] * .72f, 1);
    m_Renderer.DrawSolidRect(screen.x + 9, screen.z + 19 + bob, 7, 25, .20f, cloth[0] * .72f, cloth[1] * .72f,
                             cloth[2] * .72f, 1);
    m_Renderer.DrawSolidRect(screen.x - 7, screen.z - 4 + bob, 7, 20, 0, .11f, .07f, .04f, 1);
    m_Renderer.DrawSolidRect(screen.x + 7, screen.z - 4 + bob, 7, 20, 0, .11f, .07f, .04f, 1);
}

void WorldRenderer::DrawHouse(Vec2 position, float red, float green, float blue) const
{
    const std::vector<float> &model = m_ModelCache.LoadOrCreate("house.v1", {158, 48, 88, 76, 103, 58, 23, 43});
    const Vec2 screen = ToScreen(position);
    DrawShadow(position, 172, 36);
    m_Renderer.DrawSolidRect(screen.x + 8, screen.z - 16, model[0] * 1.05f, model[1] * 1.18f, Pi * .25f, .10f, .09f,
                             .07f, .58f);
    m_Renderer.DrawSolidRect(screen.x - 31, screen.z + 27, model[2], model[3], -.18f, .56f, .43f, .27f, 1);
    m_Renderer.DrawSolidRect(screen.x + 31, screen.z + 27, model[2], model[3], .18f, .33f, .24f, .14f, 1);
    m_Renderer.DrawSolidRect(screen.x - 28, screen.z + 80, model[4], model[5], -.30f, red * 1.14f, green * 1.10f,
                             blue * 1.04f, 1);
    m_Renderer.DrawSolidRect(screen.x + 29, screen.z + 80, model[4], model[5], .30f, red * .58f, green * .56f,
                             blue * .52f, 1);
    m_Renderer.DrawSolidRect(screen.x - 28, screen.z + 94, model[4] * .92f, 5, -.30f, red * 1.28f, green * 1.20f,
                             blue * 1.12f, .78f);
    m_Renderer.DrawSolidRect(screen.x + 29, screen.z + 94, model[4] * .92f, 5, .30f, red * .78f, green * .70f,
                             blue * .64f, .78f);
    m_Renderer.DrawSolidRect(screen.x + 34, screen.z + 8, model[6], model[7], .18f, .12f, .065f, .025f, 1);
    m_Renderer.DrawSolidRect(screen.x + 34, screen.z + 8, model[6] * .60f, model[7] * .84f, .18f, .26f, .13f, .05f, 1);
    for (int beam = -1; beam <= 1; ++beam)
        m_Renderer.DrawSolidRect(screen.x + beam * 25, screen.z + 29, 5, 69, 0, .19f, .10f, .04f, .72f);
    for (int side = -1; side <= 1; side += 2)
    {
        const float windowX = screen.x + side * 35;
        m_Renderer.DrawSolidRect(windowX, screen.z + 35, 60, 50, 0, .10f, .055f, .02f, 1);
        m_Renderer.DrawSolidRect(windowX, screen.z + 35, 39, 34, 0, 1, .42f, .08f, .13f);
        m_Renderer.DrawSolidRect(windowX, screen.z + 35, 25, 24, 0, 1, .80f, .33f, .98f);
        m_Renderer.DrawSolidRect(windowX, screen.z + 35, 4, 30, 0, .22f, .12f, .04f, .88f);
        m_Renderer.DrawSolidRect(windowX, screen.z + 35, 31, 4, 0, .22f, .12f, .04f, .88f);
    }
    m_Renderer.DrawSolidRect(screen.x + 35, screen.z + 116, 22, 52, .08f, .18f, .16f, .14f, 1);
    m_Renderer.DrawSolidRect(screen.x + 42, screen.z + 151, 25, 22, Pi * .25f, .62f, .64f, .59f, .15f);
}

void WorldRenderer::DrawTree(Vec2 position, int variant) const
{
    const std::vector<float> &model = m_ModelCache.LoadOrCreate("tree.v1", {48, 13, 10, 48, 48, 52});
    const Vec2 screen = ToScreen(position);
    const float height = 48.0f + static_cast<float>(variant) * 10.0f;
    DrawShadow(position, model[0] * 1.25f, model[1] * 1.35f);
    m_Renderer.DrawSolidRect(screen.x + 5, screen.z + height * .28f, model[2] * 1.5f, height * 1.04f, 0, .12f, .06f,
                             .02f, 1);
    m_Renderer.DrawSolidRect(screen.x - 3, screen.z + height * .30f, model[2], height, 0, .36f, .19f, .08f, 1);
    m_Renderer.DrawSolidRect(screen.x - 17, screen.z + height * .57f, 34, 7, -.48f, .24f, .12f, .04f, 1);
    m_Renderer.DrawSolidRect(screen.x + 15, screen.z + height * .70f, 30, 7, .42f, .24f, .12f, .04f, 1);
    m_Renderer.DrawSolidRect(screen.x + 9, screen.z + height + 4, model[4] * 1.22f, model[5] * 1.18f, Pi * .25f, .08f,
                             .22f, .10f, .88f);
    m_Renderer.DrawSolidRect(screen.x - 13, screen.z + height + 12, model[4] * .92f, model[5] * .86f, Pi * .25f,
                             .15f, .42f, .17f, 1);
    m_Renderer.DrawSolidRect(screen.x + 16, screen.z + height + 21, model[4] * .68f, model[5] * .62f, Pi * .25f,
                             .25f, .55f, .20f, .92f);
    m_Renderer.DrawSolidRect(screen.x - 20, screen.z + height + 27, model[4] * .42f, model[5] * .38f, Pi * .25f,
                             .36f, .66f, .25f, .72f);
}

void WorldRenderer::DrawCampfire(Vec2 position) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 20, 132, 98, Pi * .25f, 1.0f, .18f, .015f, .075f);
    for (int stone = -1; stone <= 1; ++stone)
        m_Renderer.DrawSolidRect(screen.x + stone * 18, screen.z - 2, 20, 10, Pi * .25f, .24f, .25f, .23f, 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z, 55, 17, 0, .24f, .15f, .08f, 1);
    m_Renderer.DrawEffectRect(screen.x, screen.z + 20, 30, 52, Pi * .25f, 1, .35f, .05f, .9f, 1);
    m_Renderer.DrawEffectRect(screen.x, screen.z + 18, 16, 30, Pi * .25f, 1, .88f, .20f, .95f, 1);
}

void WorldRenderer::DrawHerbPatch(Vec2 position) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 7, 42, 14, Pi * .25f, .12f, .24f, .09f, .38f);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 20, 9, 38, -.15f, .28f, .82f, .28f, 1);
}

void WorldRenderer::DrawGround() const
{
    m_Renderer.DrawSolidRect(0, 0, static_cast<float>(m_State->width), static_cast<float>(m_State->height), 0, .18f,
                             .32f, .22f, 1);
    const int centerX = static_cast<int>(std::floor(m_State->player.x));
    const int centerZ = static_cast<int>(std::floor(m_State->player.z));
    for (int z = centerZ - 14; z <= centerZ + 14; ++z)
        for (int x = centerX - 14; x <= centerX + 14; ++x)
        {
            const Vec2 position = {static_cast<float>(x), static_cast<float>(z)};
            float red = .24f;
            float green = .47f;
            float blue = .25f;
            const float grassVariation = std::sin(position.x * 7.73f + position.z * 4.19f) * .026f;
            red += grassVariation * .45f;
            green += grassVariation;
            blue += grassVariation * .35f;
            const float road = RoadDistance(position);
            if (Distance(position, m_State->world.lake) < m_State->world.lakeRadius + .5f)
            {
                red = .10f; green = .38f; blue = .55f;
            }
            else if (road < 2.0f || Distance(position, {0, 0}) < 4.5f)
            {
                red = road < .58f ? .50f : .39f;
                green = road < .58f ? .44f : .34f;
                blue = road < .58f ? .30f : .23f;
            }
            DrawRect(position, TileWidth * .72f, TileHeight * .72f, Pi * .25f, red, green, blue);
            if (road > 2.0f && !((x + z) % 3) &&
                Distance(position, m_State->world.lake) > m_State->world.lakeRadius + 1.0f)
                DrawRect({position.x + .18f, position.z - .15f}, 7, 18, -.18f, .20f, .52f, .18f, .38f);
        }
    for (int index = 0; index < 8; ++index)
    {
        const float offset = static_cast<float>(index - 4);
        const Vec2 wave = {m_State->world.lake.x + offset * 1.2f, m_State->world.lake.z + std::sin(m_State->time + offset)};
        const Vec2 screen = ToScreen(wave);
        m_Renderer.DrawEffectRect(screen.x, screen.z, 48, 12, 0, .48f, .82f, .92f, .50f, 2);
    }
}

void WorldRenderer::RenderWorld(const GameState &state)
{
    m_State = &state;
    DrawGround();
    for (const Vec2 &house : std::array<Vec2, 8>{{{-14, 5}, {15, 5}, {-9, 19}, {10, -20}, {-27, -14}, {27, -14}, {-25, 20}, {26, 19}}})
        DrawHouse(house, .42f, .18f, .09f);
    for (int index = 0; index < 32; ++index)
        DrawTree({4 + std::cos(index * 1.71f) * (43.0f + (index % 6) * 6.5f),
                  7 + std::sin(index * 1.71f) * (43.0f + (index % 6) * 6.5f)}, index % 4);
    const std::array<Vec2, 10> campfires = {
        {{-3, 0}, {8, 2}, {-9, -8}, {12, -12}, {-13, 12}, {16, 12}, {-24, 0}, {25, 1}, {0, 24}, {1, -25}}};
    for (const Vec2 &campfire : campfires)
        DrawCampfire(campfire);
    for (const NpcState &npc : state.npcs)
        DrawSprite(npc.position, npc.column, npc.row, 88, 112);
    for (const AnimalState &animal : state.animals)
        DrawSprite(animal.position, animal.column, animal.row, 105, 82);
    if (state.quest == QuestStage::Tracks)
        DrawRect(state.world.tracks, 30, 9, Pi * .2f, .12f, .07f, .03f);
    if (state.quest == QuestStage::Herb)
        DrawHerbPatch(state.world.herb);
    if (state.quest == QuestStage::Hunt || state.quest == QuestStage::Trophy)
    {
        MonsterDrawContext context = {&m_Renderer, [this](Vec2 p) { return ToScreen(p); },
                                      [this](Vec2 p, int c, int r, float w, float h, bool s) { DrawSprite(p, c, r, w, h, s); },
                                      state.width, state.height, state.time, Pi};
        MonsterSystem::Draw(state.monster, context, state.quest == QuestStage::Trophy);
    }
    DrawSprite(state.player, 0, 0, 42, 54);
    m_State = nullptr;
}
