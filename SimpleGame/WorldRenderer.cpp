#include "stdafx.h"
#include "WorldRenderer.h"

#include <GL/glew.h>
#include "Renderer.h"
#include "WorldLayout.h"
#include "StorySystem.h"
#include "PixelNpcFrames.h"
#include "PixelAnimalFrames.h"
#include "Chapter1BoarAnimation.h"
#include "DamageFeedback.h"
#include "PixelGeneralFrames.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
constexpr float Pi = 3.14159265f;
constexpr float TileWidth = 64.0f;
constexpr float TileHeight = 32.0f;
constexpr float WorldScale = 2.6f;
constexpr float GroundDotSize = 2.0f;
constexpr int GroundChunkDots = 48;
constexpr float GroundChunkSize = GroundDotSize * GroundChunkDots;
constexpr float HumanPivotY = .19070485f;
constexpr float AnimalPivotY = .2720983f;
const GameContent::MapDefinition &Map() { return GameContent::Get().map; }

void ApplyLocalLight(Renderer &renderer, Vec2 position, float time)
{
    float strength = 0;
    for (size_t index = 0; index < Map().campfires.size(); ++index)
    {
        const float x = position.x - Map().campfires[index].position.x, z = position.z - Map().campfires[index].position.z;
        const float falloff = std::max(0.0f, 1.0f - std::sqrt(x*x + z*z) / 4.5f);
        strength += falloff * falloff * (.94f + .06f * std::sin(time * 6 + static_cast<float>(index)));
    }
    renderer.SetLocalLight(strength * .90f, strength * .28f, strength * .055f);
}

unsigned int LoadFantasyTexture(Renderer &renderer, const wchar_t *name)
{
    const std::wstring fantasyPath = std::wstring(L"./Assets/Textures/Fantasy/") + name;
    const unsigned int texture = renderer.LoadTexture(fantasyPath.c_str());
    return texture != 0 ? texture : renderer.LoadTexture((std::wstring(L"./Assets/Textures/") + name).c_str());
}

// Local screen geometry keeps the existing quarter-view world projection.
void DrawBlock(Renderer &renderer, float x, float y, float width, float height,
               float red, float green, float blue)
{
    renderer.DrawEffectRect(x, y, width, height, 0, red, green, blue, 1, 4);
}

void DrawCanopy(Renderer &renderer, float x, float y, float width, float height,
                float red, float green, float blue)
{
    renderer.DrawEffectRect(x, y, width, height, 0, red, green, blue, 1, 5);
}

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
    const float lengthSquared = std::max(.0001f, dx * dx + dz * dz);
    const float progress = std::max(0.0f, std::min(1.0f, ((position.x - start.x) * dx + (position.z - start.z) * dz) /
                                                        lengthSquared));
    return Distance(position, {start.x + dx * progress, start.z + dz * progress});
}

float RoadDistance(Vec2 position)
{
    const auto mainRoadX = [](float z) { return 3.5f * std::sin(z * .065f); };
    const float roadZ = std::max(-55.0f, std::min(65.0f, position.z));
    const float mainWidth = 1.55f + .22f * std::cos(roadZ * .11f);
    float distance = Distance(position, {mainRoadX(roadZ), roadZ}) - mainWidth;

    // Each lane leaves the main road at a different point and narrows toward its front door.
    for (const auto &house : Map().houses)
    {
        const Vec2 start = {mainRoadX(house.junction), house.junction};
        const Vec2 door = {house.position.x + .78f, house.position.z - .28f};
        const float taper = std::max(0.0f, std::min(1.0f, Distance(position, door) / std::max(.01f, Distance(start, door))));
        const float width = .48f + .44f * taper;
        distance = std::min(distance, DistanceToSegment(position, start, house.roadBend) - width);
        distance = std::min(distance, DistanceToSegment(position, house.roadBend, door) - width);
    }

    const float plazaX = (position.x - Map().noticeBoard.x) / 3.4f;
    const float plazaZ = (position.z - (Map().noticeBoard.z - 2.0f)) / 2.6f;
    const float plaza = (std::sqrt(plazaX * plazaX + plazaZ * plazaZ) - 1.0f) * 2.6f;
    return std::min(distance, plaza);
}
}

WorldRenderer::WorldRenderer(Renderer &renderer) : m_Renderer(renderer)
{
    m_TreeTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/tree-v1.png");
    m_PixelTree = m_TreeTexture != 0;
    if (m_PixelTree)
        m_TreePivotY = .09344f;
    else
    {
        m_TreeTexture = m_Renderer.LoadTexture(L"./Assets/Textures/Fantasy/tree.png");
        if (m_TreeTexture != 0)
            m_TreePivotY = .18508148f;
        else
            m_TreeTexture = m_Renderer.LoadTexture(L"./Assets/Textures/tree.png");
    }
    m_TerrainTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/terrain-v1.png");
    m_PixelTerrain = m_TerrainTexture != 0;
    if (!m_PixelTerrain)
        m_TerrainTexture = LoadFantasyTexture(m_Renderer, L"terrain_atlas.png");
    m_HumanTexture = m_Renderer.LoadTexture(L"./Assets/Frontier/Textures/human_atlas.png");
    if (m_HumanTexture == 0)
        m_HumanTexture = LoadFantasyTexture(m_Renderer, L"human_atlas.png");
    m_BoarTexture = LoadFantasyTexture(m_Renderer, L"boar_atlas.png");
    m_DeerTexture = LoadFantasyTexture(m_Renderer, L"deer_atlas.png");
    m_SheepTexture = LoadFantasyTexture(m_Renderer, L"sheep_atlas.png");
    m_PixelAnimalTextures[0] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Animals/deer-walk-v1.png");
    m_PixelAnimalTextures[1] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Animals/boar-walk-v1.png");
    m_PixelAnimalTextures[2] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Animals/sheep-walk-v1.png");
    m_HouseTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/house-v1.png");
    const bool pixelHouse = m_HouseTexture != 0;
    if (pixelHouse)
        m_HousePivotY = .10f;
    else
        m_HouseTexture = m_Renderer.LoadTexture(L"./Assets/Textures/Fantasy/house.png");
    m_PropsTexture = m_Renderer.LoadTexture(L"./Assets/Frontier/Textures/props_atlas.png");
    m_SceneryTexture = m_Renderer.LoadTexture(L"./Assets/Frontier/Textures/scenery_atlas.png");
    m_PlayerTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/hero-walk-v1.png");
    m_GeneralWalkTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/hero-general-walk-v2.png");
    m_GeneralRunTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/hero-general-v3.png");
    m_GeneralAttackTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/hero-general-attack-v1.png");
    m_PixelPlayer = m_PlayerTexture != 0;
    if (!m_PixelPlayer)
    {
        m_PlayerTexture = m_Renderer.LoadTexture(L"./Assets/Hero/Textures/human_atlas.png");
        m_PlayerAttackTexture = m_Renderer.LoadTexture(L"./Assets/Hero/Textures/human_attack_atlas.png");
    }
    m_SwordSlashTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Effects/sword-slash.png");
    m_ElderTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/elder-v1.png");
    m_NoticeBoardTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/notice-board-v1.png");
    m_RockTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/rock-v1.png");
    m_PineTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/pine-v1.png");
    m_NpcTextures[0] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/npc-farmer-v1.png");
    m_NpcTextures[1] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Characters/npc-merchant-v1.png");
    m_HouseVariants[0] = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/house-stone-v1.png");
    m_HouseVariants[1] = m_Renderer.LoadTexture(L"./Assets/PixelArt/World/house-shop-v1.png");
    m_CampfireTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Effects/campfire.png");
    m_PlayerHitTexture = m_Renderer.LoadTexture(L"./Assets/PixelArt/Effects/boar-player-hit.png");
    m_AttackHitTextures[0] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Effects/attack-hit-spark.png");
    m_AttackHitTextures[1] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Effects/attack-hit-heavy.png");
    m_AttackHitTextures[2] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Effects/attack-hit-gold.png");
    m_Chapter1BoarTextures[0] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Monsters/Chapter1/great-boar-attack-v1.png");
    m_Chapter1BoarTextures[1] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Monsters/Chapter1/great-boar-hit-v1.png");
    m_Chapter1BoarTextures[2] = m_Renderer.LoadTexture(L"./Assets/PixelArt/Monsters/Chapter1/great-boar-death-v1.png");
    if (m_SwordSlashTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, m_SwordSlashTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    // Atlas cells share a texture: avoid mipmaps blending neighboring directions/tiles.
    for (unsigned int texture : {m_SceneryTexture, m_PropsTexture, m_TerrainTexture, m_HumanTexture, m_PlayerTexture, m_PlayerAttackTexture, m_BoarTexture, m_DeerTexture, m_SheepTexture})
        if (texture != 0)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    // New pixel assets keep their hard edges at both magnification and minification.
    for (unsigned int texture : {m_PixelTree ? m_TreeTexture : 0u, pixelHouse ? m_HouseTexture : 0u,
                                 m_PixelTerrain ? m_TerrainTexture : 0u, m_PixelPlayer ? m_PlayerTexture : 0u,
                                 m_ElderTexture, m_NoticeBoardTexture, m_RockTexture, m_PineTexture,
                                 m_NpcTextures[0], m_NpcTextures[1], m_HouseVariants[0], m_HouseVariants[1],
                                 m_CampfireTexture, m_GeneralWalkTexture, m_GeneralRunTexture, m_GeneralAttackTexture,
                                 m_PixelAnimalTextures[0], m_PixelAnimalTextures[1], m_PixelAnimalTextures[2],
                                 m_Chapter1BoarTextures[0], m_Chapter1BoarTextures[1], m_Chapter1BoarTextures[2], m_PlayerHitTexture,
                                 m_AttackHitTextures[0], m_AttackHitTextures[1], m_AttackHitTextures[2]})
        if (texture != 0)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    glBindTexture(GL_TEXTURE_2D, 0);
}

WorldRenderer::~WorldRenderer()
{
    if (m_PlayerHitTexture != 0) glDeleteTextures(1, &m_PlayerHitTexture);
    for (unsigned int texture : m_AttackHitTextures)
        if (texture != 0) glDeleteTextures(1, &texture);
    for (unsigned int texture : m_Chapter1BoarTextures)
        if (texture != 0)
            glDeleteTextures(1, &texture);
    for (unsigned int texture : m_PixelAnimalTextures)
        if (texture != 0)
            glDeleteTextures(1, &texture);
    for (unsigned int texture : {m_GeneralWalkTexture, m_GeneralRunTexture, m_GeneralAttackTexture})
        if (texture != 0)
            glDeleteTextures(1, &texture);
    for (unsigned int texture : {m_NpcTextures[0], m_NpcTextures[1], m_HouseVariants[0], m_HouseVariants[1],
                                 m_CampfireTexture})
        if (texture != 0)
            glDeleteTextures(1, &texture);
    for (unsigned int texture : {m_ElderTexture, m_NoticeBoardTexture, m_RockTexture, m_PineTexture})
        if (texture != 0)
            glDeleteTextures(1, &texture);
    if (m_SwordSlashTexture != 0)
        glDeleteTextures(1, &m_SwordSlashTexture);
    for (unsigned int texture : {m_SceneryTexture, m_PropsTexture, m_TreeTexture, m_HouseTexture, m_PlayerTexture, m_PlayerAttackTexture, m_TerrainTexture, m_HumanTexture, m_BoarTexture, m_DeerTexture, m_SheepTexture})
        if (texture != 0)
            glDeleteTextures(1, &texture);
    if (m_GroundCache.buffer != 0)
        glDeleteBuffers(1, &m_GroundCache.buffer);
}

void WorldRenderer::BeginFrame(float time)
{
    m_Renderer.BeginFrame(time);
    m_Renderer.SetWorldScale(WorldScale);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void WorldRenderer::EndFrame(float time)
{
    m_Renderer.EndFrame(time);
    m_Renderer.SetWorldScale(1.0f);
}

Vec2 WorldRenderer::ToScreen(Vec2 position) const
{
    const float x = position.x - m_State->player.x;
    const float z = position.z - m_State->player.z;
    return {(x - z) * TileWidth * .5f, (x + z) * TileHeight * .5f - 86.0f};
}

void WorldRenderer::DrawRect(Vec2 position, float width, float height, float rotation, float red, float green,
                             float blue, float alpha) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawSolidRect(screen.x, screen.z, width, height, rotation, red, green, blue, alpha);
}

void WorldRenderer::DrawShadow(Vec2 position, float width, float height, float castHeight) const
{
    const Vec2 screen = ToScreen(position);
    // Shared cast direction, with a darker contact patch at the object's ground anchor.
    m_Renderer.DrawEffectRect(screen.x + width * .22f + castHeight * .18f, screen.z - height * .45f - castHeight * .06f,
                              width * 1.45f + castHeight * .42f, height * 2.2f + castHeight * .18f,
                              -.35f, .025f, .035f, .045f, .22f, 3);
    m_Renderer.DrawEffectRect(screen.x, screen.z, width * .72f, height * .75f,
                              0, .02f, .025f, .03f, .38f, 3);
}

bool WorldRenderer::DrawAtlasSprite(Vec2 position, unsigned int texture, Vec2 facing, int frame,
                                   float height, float pivotY, bool fallen) const
{
    if (texture == 0)
        return false;
    // Blender (X,Y) maps to game (x,z)=(Y,-X); rotated front -Y becomes (-cos(a),-sin(a)).
    const int direction = (static_cast<int>(std::lround(std::atan2(-facing.z, -facing.x) / (Pi * .25f))) + 8) % 8;
    const Vec2 screen = ToScreen(position);
    const float width = height * (128.0f / 192.0f);
    const float displayHeight = fallen ? height * .45f : height;
    DrawShadow(position, width * .6f, fallen ? 12.0f : 9.0f);
    const float tint = fallen ? .6f : 1.0f;
    m_Renderer.DrawSprite(texture, screen.x, screen.z + (.5f - pivotY) * displayHeight,
                          width, displayHeight, direction, fallen ? 0 : frame % 4, 8, 4, 1, tint, tint, tint);
    return true;
}

void WorldRenderer::DrawSprite(Vec2 position, int column, int row, float width, float height, bool shadow) const
{
    const Vec2 screen = ToScreen(position);
    const bool animal = row >= 2;
    const float scale = animal ? std::max(.8f, std::min(1.3f, width / 105.0f))
                               : std::max(1.0f, std::min(1.25f, height / 72.0f));
    if (shadow)
        DrawShadow(position, (animal ? 58.0f : 34.0f) * scale, 12.0f * scale);
    const auto block = [&](float x, float y, float w, float h, float r, float g, float b)
    {
        DrawBlock(m_Renderer, screen.x + x * scale, screen.z + y * scale, w * scale, h * scale, r, g, b);
    };
    if (animal)
    {
        const float red = column == 0 ? .55f : (column == 1 ? .27f : .64f);
        const float green = column == 0 ? .34f : (column == 1 ? .16f : .42f);
        const float blue = column == 2 ? .22f : .09f;
        block(-13, 5, 7, 17, .17f, .10f, .05f);
        block(12, 6, 7, 17, .17f, .10f, .05f);
        block(0, 20, 46, 32, red, green, blue);
        block(-13, 1, 7, 18, .22f, .13f, .06f);
        block(12, 2, 7, 18, .22f, .13f, .06f);
        block(23, 22, 21, 24, red * .9f, green * .9f, blue * .9f);
        block(33, 16, 14, 11, red * .7f, green * .7f, blue * .7f);
        block(18, 35, 6, 12, red * .7f, green * .7f, blue * .7f);
        block(29, 35, 6, 12, red * .7f, green * .7f, blue * .7f);
        block(32, 26, 3, 4, .025f, .02f, .015f);
        return;
    }
    const float colors[4][3] = {{.12f, .28f, .16f}, {.48f, .16f, .08f}, {.10f, .24f, .42f}, {.36f, .12f, .30f}};
    const float *cloth = colors[column % 4];
    const bool player = position.x == m_State->player.x && position.z == m_State->player.z;
    const float stride = player && m_State->moving ? std::sin(m_State->time * (m_State->sprint ? 9.0f : 6.5f)) : 0;
    const float bob = player ? std::abs(stride) * 1.1f : std::sin(m_State->time * 2.0f + column) * .4f;
    block(-6, 7 + stride * 2, 9, 19, .12f, .08f, .05f);
    block(6, 7 - stride * 2, 9, 19, .12f, .08f, .05f);
    block(-13, 22 + bob - stride, 7, 20, cloth[0] * .8f, cloth[1] * .8f, cloth[2] * .8f);
    block(0, 25 + bob, 23, 30, cloth[0], cloth[1], cloth[2]);
    block(0, 15 + bob, 24, 7, .27f, .16f, .07f);
    block(13, 22 + bob + stride, 7, 20, cloth[0], cloth[1], cloth[2]);
    block(0, 47 + bob, 17, 19, .74f, .52f, .32f);
    block(0, 55 + bob, 20, 11, .17f, .10f, .05f);
    const float look = player && m_State->facing.x < m_State->facing.z ? -1.0f : 1.0f;
    block(look * 4, 47 + bob, 3, 3, .025f, .02f, .015f);
    block(look * 8, 43 + bob, 4, 4, .62f, .39f, .23f);
}

void WorldRenderer::DrawPixelSprite(Vec2 position, unsigned int texture, float width, float height,
                                    float pivotY, float sway) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawSprite(texture, screen.x, screen.z + (.5f - pivotY) * height,
                          width, height, 0, 0, 1, 1, 1, 1, 1, 1, sway, pivotY, 0, 1, 1, .5f);
}

void WorldRenderer::DrawGeneralPlayer(const GameState &state) const
{
    const bool running = state.moving && state.sprint && state.attack <= 0.0f && m_GeneralRunTexture != 0;
    const int mode = running ? 1 : 0;
    const int worldDirection = (static_cast<int>(std::lround(std::atan2(-state.facing.z, -state.facing.x) /
                                                                        (Pi * .25f))) + 8) % 8;
    const int direction = (10 - worldDirection) % 8;
    const float progress = std::max(0.0f, std::min(1.0f, 1.0f - state.attack / .32f));
    const int frame = state.attack > 0 ? std::min(3, static_cast<int>(progress * 4)) :
                      (state.moving ? static_cast<int>(state.time * (state.sprint ? 8 : 6)) % 4 : 0);
    const bool newSheet = m_GeneralRunTexture != 0 && state.attack <= 0.0f && (running || !state.moving);
    // Atlas row order from general-v3/manifest.json: down,side,up,se,sw,ne,nw,left.
    constexpr int directionRows[8] = {9, 0, 12, 21, 18, 6, 15, 3};
    const int atlasFrame = running ? frame : static_cast<int>(state.time * 4) % 4;
    const int atlasRow = directionRows[direction] + (running ? 2 : 0);
    const bool attacking = state.attack > 0.0f && m_GeneralAttackTexture != 0;
    const int attackX = frame * 887 / 4, attackY = direction * 1774 / 8;
    const PixelGeneralFrames::Frame region = attacking ?
        PixelGeneralFrames::Frame{attackX, attackY, (frame + 1) * 887 / 4 - attackX,
                                 (direction + 1) * 1774 / 8 - attackY,
                                 (frame + .5f) * 887 / 4, attackY + 215.0f} : newSheet ?
        PixelGeneralFrames::Frame{atlasFrame * 192, atlasRow * 256, 192, 256,
                                 atlasFrame * 192 + 96.0f, atlasRow * 256 + 236.0f} :
        PixelGeneralFrames::Frames[mode][frame][direction];
    // Measured v3 crown hair area (top 45px, alpha >= .5), median of four poses per direction.
    // Scale = idle scale * sqrt(idle area / run area); fixed across phases to preserve bent-knee motion.
    constexpr float runScale[8] = {
        .293683f, .302495f, .358914f, .338193f,
        .351701f, .333751f, .369871f, .331619f
    };
    const float scale = attacking ? 74.0f / 177.0f : running ? runScale[direction] * (direction % 2 == 0 ? 1.15f : 1.0f) :
                        newSheet ? 74.0f / 216.0f : PixelGeneralFrames::Scale[mode];
    const Vec2 foot = ToScreen(state.player);
    const float pulse = state.attack > 0 ? std::sin(progress * Pi) : 0;
    const float dx = state.facing.x - state.facing.z, dy = (state.facing.x + state.facing.z) * .5f;
    const float length = std::max(.001f, std::sqrt(dx * dx + dy * dy));
    const float pivotY = (region.y + region.height - region.footY) / region.height;
    DrawShadow(state.player, 35, 9);
    m_Renderer.DrawSprite(attacking ? m_GeneralAttackTexture : newSheet ? m_GeneralRunTexture : m_GeneralWalkTexture,
                          foot.x + (region.x + region.width * .5f - region.footX) * scale + dx / length * pulse * 3,
                          foot.z + (region.footY - region.y - region.height * .5f) * scale + dy / length * pulse * 3,
                          region.width * scale, region.height * scale, region.x, region.y,
                          attacking ? 887 : newSheet ? 768 : PixelGeneralFrames::Width[mode],
                          attacking ? 1774 : newSheet ? 6144 : PixelGeneralFrames::Height[mode],
                          1, 1, 1, 1, pulse * .025f, pivotY, 0,
                          static_cast<float>(region.width), static_cast<float>(region.height), .5f);
}

void WorldRenderer::DrawPixelPlayer(const GameState &state) const
{
    const int worldDirection = (static_cast<int>(std::lround(std::atan2(-state.facing.z, -state.facing.x) /
                                                                        (Pi * .25f))) + 8) % 8;
    // Pixel sheet runs SE,S,SW,W,NW,N,NE,E; reflect the Blender ordering horizontally.
    const int direction = (10 - worldDirection) % 8;
    const float progress = std::max(0.0f, std::min(1.0f, 1.0f - state.attack / .32f));
    const int frame = state.attack > 0 ? std::min(3, static_cast<int>(progress * 4)) :
                      (state.moving ? static_cast<int>(state.time * (state.sprint ? 8 : 6)) % 4 : 0);
    // Alpha-measured foot anchors for the generated 8x4 sheet (top-to-bottom rows).
    static constexpr Vec2 pivots[4][8] = {
        {{.5994f,.0664f},{.5000f,.0664f},{.5994f,.0664f},{.5221f,.0664f},
         {.4641f,.0664f},{.4448f,.0664f},{.4088f,.0664f},{.3729f,.0775f}},
        {{.5110f,.0809f},{.4945f,.0882f},{.6188f,.0809f},{.5359f,.0809f},
         {.4890f,.0809f},{.4503f,.0882f},{.3950f,.0882f},{.3674f,.0846f}},
        {{.5497f,.0664f},{.4530f,.0664f},{.6133f,.0664f},{.5608f,.0664f},
         {.5276f,.0664f},{.4475f,.0664f},{.3398f,.0664f},{.4061f,.0849f}},
        {{.4945f,.0956f},{.4890f,.1250f},{.4917f,.0956f},{.5221f,.0956f},
         {.4724f,.0956f},{.4088f,.1140f},{.3646f,.0956f},{.4448f,.1103f}}
    };
    // Individual opaque bounds avoid cape pixels spilling into neighboring directions.
    static constexpr int regions[4][8][4] = {
        {{43,28,130,225},{223,26,124,227},{396,29,139,224},{571,29,132,224},
         {745,27,134,226},{929,26,119,227},{1088,27,146,226},{1267,28,131,222}},
        {{41,300,126,221},{226,298,121,221},{407,301,132,220},{577,300,123,221},
         {752,298,125,223},{930,298,112,221},{1088,299,144,220},{1263,300,135,220}},
        {{41,571,132,225},{222,569,124,227},{397,571,141,225},{578,570,127,226},
         {750,569,129,227},{929,569,117,227},{1087,570,147,226},{1269,570,129,221}},
        {{36,843,131,217},{228,841,119,211},{410,843,129,217},{581,843,119,217},
         {753,841,127,219},{929,841,117,214},{1088,842,145,218},{1263,843,135,213}}
    };
    const Vec2 pivot = pivots[frame][direction], foot = ToScreen(state.player);
    const int *region = regions[frame][direction];
    constexpr float scale = 88.0f / 271.5f;
    const float width = region[2] * scale, height = region[3] * scale;
    const float anchorX = (direction + pivot.x) * 181.0f;
    const float anchorY = (frame + 1 - pivot.z) * 271.5f;
    const float pivotFromBottom = (region[1] + region[3] - anchorY) / region[3];
    const float pulse = state.attack > 0 ? std::sin(progress * Pi) : 0;
    const float dx = state.facing.x - state.facing.z, dy = (state.facing.x + state.facing.z) * .5f;
    const float length = std::max(.001f, std::sqrt(dx * dx + dy * dy));
    DrawShadow(state.player, 35, 9);
    m_Renderer.DrawSprite(m_PlayerTexture,
                          foot.x + (region[0] + region[2] * .5f - anchorX) * scale + dx / length * pulse * 3,
                          foot.z + (anchorY - region[1] - region[3] * .5f) * scale + dy / length * pulse * 3,
                          width, height, region[0], region[1], 1448, 1086, 1, 1, 1, 1,
                          pulse * .025f, pivotFromBottom, 0, static_cast<float>(region[2]),
                          static_cast<float>(region[3]));
}

bool WorldRenderer::DrawPixelAnimal(Vec2 position, int species, Vec2 facing, int frame,
                                    float height, bool fallen) const
{
    const unsigned int texture = m_PixelAnimalTextures[species];
    if (texture == 0)
        return false;
    const float dx = facing.x - facing.z, dy = (facing.x + facing.z) * .5f;
    const int direction = std::abs(dx) > std::abs(dy) ? (dx < 0 ? 1 : 2) : (dy < 0 ? 0 : 3);
    const Vec2 screen = ToScreen(position);
    const float displayHeight = fallen ? height * .45f : height;
    const float tint = fallen ? .6f : 1.0f;
    DrawShadow(position, height * .55f, fallen ? 12.0f : 9.0f);
    const int pose = fallen ? 1 : frame % 4;
    const auto pivot = PixelAnimalFrames::Pivots[species][pose][direction];
    m_Renderer.DrawSprite(texture, screen.x + (.5f - pivot.x) * height,
                          screen.z + (pivot.y - .5f) * displayHeight,
                          height, displayHeight, direction, pose, 4, 4,
                          1, tint, tint, tint, 0, 0, 0, 1, 1, .5f);
    return true;
}

bool WorldRenderer::DrawChapter1Boar(const MonsterState &monster) const
{
    if (monster.definition.id != "rampaging_boar") return false;
    using namespace Chapter1BoarAnimation;
    Action action = Attack;
    float elapsed = 0.0f;
    if (monster.health <= 0)
    {
        action = Death;
        elapsed = monster.deathTime > 0 ? monster.deathTime : Duration(Death);
    }
    else if (monster.hitAnimation > 0)
    {
        action = Hit;
        elapsed = Duration(Hit) - monster.hitAnimation;
    }
    else if (monster.attackAnimation > 0)
        elapsed = Duration(Attack) - monster.attackAnimation;
    const unsigned int texture = m_Chapter1BoarTextures[action];
    if (texture == 0) return false;
    const Vec2 facing = monster.animationFacing;
    const float dx = facing.x - facing.z, dy = (facing.x + facing.z) * .5f;
    const int direction = std::abs(dx) > std::abs(dy) ? (dx < 0 ? 0 : 1) : (dy < 0 ? 2 : 3);
    constexpr float anchorX[] = {131.5f, 137.0f, 126.0f, 128.5f};
    constexpr float anchorY[] = {225, 224, 224, 233};
    const float height = monster.definition.spriteHeight;
    const Vec2 screen = ToScreen(monster.position);
    DrawShadow(monster.position, height * .55f, 12);
    m_Renderer.DrawSprite(texture, screen.x + (.5f - anchorX[direction] / 256) * height,
                          screen.z + (anchorY[direction] / 256 - .5f) * height,
                          height, height, Frame(action, elapsed), direction, 6, 4,
                          1, 1, 1, 1, 0, 0, 0, 1, 1, .5f);
    return true;
}

void WorldRenderer::DrawPixelNpc(const NpcState &npc, int variant) const
{
    const float dx = npc.facing.x - npc.facing.z, dy = (npc.facing.x + npc.facing.z) * .5f;
    const int direction = std::abs(dx) > std::abs(dy) ? (dx < 0 ? 1 : 2) : (dy < 0 ? 0 : 3);
    const int frame = npc.moving ? (static_cast<int>(npc.phase) % 4 + 4) % 4 : 1;
    const auto &region = PixelNpcFrames::Frames[variant][frame][direction];
    const float scale = PixelNpcFrames::Scale[variant];
    const Vec2 screen = ToScreen(npc.position);
    DrawShadow(npc.position, 32, 9);
    m_Renderer.DrawSprite(m_NpcTextures[variant],
                          screen.x + (region.x + region.width * .5f - region.footX) * scale,
                          screen.z + (region.footY - region.y - region.height * .5f) * scale,
                          region.width * scale, region.height * scale, region.x, region.y,
                          PixelNpcFrames::Width[variant], PixelNpcFrames::Height[variant],
                          1, 1, 1, 1, 0, 0, 0, static_cast<float>(region.width),
                          static_cast<float>(region.height), .5f);
}

void WorldRenderer::DrawHouse(Vec2 position, float red, float green, float blue, int variant) const
{
    const int style = variant % 3;
    if (style > 0 && m_HouseVariants[style - 1] != 0)
    {
        DrawShadow(position, 172, 36, 120);
        DrawPixelSprite(position, m_HouseVariants[style - 1], 220, 220, .10f);
        return;
    }
    if (m_HouseTexture != 0)
    {
        const Vec2 screen = ToScreen(position);
        DrawShadow(position, 172, 36, 120);
        m_Renderer.DrawSprite(m_HouseTexture, screen.x, screen.z + (.5f-m_HousePivotY)*220,
                              220, 220, 0, 0, 1, 1);
        return;
    }
    const std::vector<float> &model = m_ModelCache.LoadOrCreate("house.v1", {158, 48, 88, 76, 103, 58, 23, 43});
    const Vec2 screen = ToScreen(position);
    DrawShadow(position, 172, 36, 120);
    DrawBlock(m_Renderer, screen.x, screen.z + 40, model[0], model[3] + 24, .56f, .43f, .27f);
    DrawCanopy(m_Renderer, screen.x, screen.z + 91, model[0] + 26, model[5] + 26, red, green, blue);
    DrawBlock(m_Renderer, screen.x + 36, screen.z + 107, 21, 45, .27f, .24f, .20f);
    DrawBlock(m_Renderer, screen.x + 36, screen.z + 127, 28, 10, .36f, .33f, .29f);
    DrawBlock(m_Renderer, screen.x + 26, screen.z + 12, model[6], model[7], .23f, .12f, .045f);
    DrawBlock(m_Renderer, screen.x + 31, screen.z + 11, 3, 4, .70f, .54f, .23f);
    DrawBlock(m_Renderer, screen.x - 27, screen.z + 42, 26, 28, .18f, .11f, .055f);
    DrawBlock(m_Renderer, screen.x - 27, screen.z + 43, 19, 19, .86f, .60f, .24f);
    m_Renderer.DrawSolidRect(screen.x - 27, screen.z + 43, 2, 15, 0, .25f, .14f, .065f, 1);
    m_Renderer.DrawSolidRect(screen.x - 27, screen.z + 43, 16, 2, 0, .25f, .14f, .065f, 1);
}

void WorldRenderer::DrawTree(Vec2 position, int variant) const
{
    const Vec2 screen = ToScreen(position);
    if (m_TreeTexture != 0)
    {
        // Export pivot is measured from the image bottom; keep the trunk at the world anchor.
        // Each version's pivot comes from its tree.sprite.json (512 x 640).
        const float spriteHeight = 148.0f + static_cast<float>(variant) * 13.0f;
        const float spriteWidth = spriteHeight * .8f;
        DrawShadow(position, spriteWidth * .65f, 17.0f, spriteHeight * .7f);
        m_Renderer.DrawSprite(m_TreeTexture, screen.x, screen.z + (.5f - m_TreePivotY) * spriteHeight,
                              spriteWidth, spriteHeight, 0, 0, 1, 1, 1,
                              m_PixelTree ? 1.0f : .76f - variant * .015f,
                              m_PixelTree ? 1.0f : .83f, m_PixelTree ? 1.0f : .68f - variant * .02f,
                              std::sin(m_State->time * 1.4f + position.x * .2f + position.z * .3f) * .025f,
                              m_TreePivotY);
        return;
    }
    const std::vector<float> &model = m_ModelCache.LoadOrCreate("tree.v1", {48, 13, 10, 48, 48, 52});
    const float height = 48.0f + static_cast<float>(variant) * 10.0f;
    DrawShadow(position, model[0] * 1.4f, model[1] * 1.6f, height);
    DrawBlock(m_Renderer, screen.x, screen.z + height * .4f, model[2] * 1.8f, height, .36f, .19f, .08f);
    const float sway = std::sin(m_State->time * 1.4f + position.x * .2f + position.z * .3f) * 1.2f;
    DrawCanopy(m_Renderer, screen.x + sway, screen.z + height * .8f, model[4] * 1.6f, model[5], .12f, .34f, .14f);
    DrawCanopy(m_Renderer, screen.x - 9 + sway, screen.z + height + 12, model[4] * 1.3f, model[5] * .9f, .20f, .46f, .18f);
    DrawCanopy(m_Renderer, screen.x + 8 + sway, screen.z + height + 31, model[4], model[5] * .75f, .30f, .57f, .22f);
}

void WorldRenderer::DrawCampfire(Vec2 position) const
{
    const Vec2 screen = ToScreen(position);
    const float pulse = .82f + std::sin(m_State->time * 7.0f) * .08f;
    m_Renderer.DrawEffectRect(screen.x, screen.z + 17, 174, 118, Pi * .25f, 1.0f, .20f, .02f, .17f * pulse, 3);
    m_Renderer.DrawEffectRect(screen.x, screen.z + 20, 132, 98, Pi * .25f, 1.0f, .18f, .015f, .12f * pulse, 3);
    m_Renderer.DrawEffectRect(screen.x, screen.z, 62, 24, 0, .09f, .065f, .04f, .7f, 3);
    if (m_CampfireTexture != 0)
    {
        const int frame = static_cast<int>(m_State->time * 12.0f + std::abs(position.x * 3 + position.z * 5)) % 36;
        m_Renderer.DrawSprite(m_CampfireTexture, screen.x, screen.z + (.5f - .125f) * 160,
                              160, 160, frame % 6, frame / 6, 6, 6);
        return;
    }
    for (int stone = 0; stone < 8; ++stone)
    {
        const float angle = stone * Pi * .25f;
        m_Renderer.DrawEffectRect(screen.x + std::cos(angle) * 24, screen.z + std::sin(angle) * 8,
                                  13, 10, angle * .1f, .32f, .31f, .27f, 1, 17);
    }
    DrawBlock(m_Renderer, screen.x - 6, screen.z + 2, 33, 8, .22f, .12f, .055f);
    DrawBlock(m_Renderer, screen.x + 7, screen.z + 5, 29, 7, .28f, .16f, .075f);
    // Keep flames upright; the shader forms the hot core and rising embers.
    m_Renderer.DrawEffectRect(screen.x, screen.z + 32, 48, 72, 0, 1, .35f, .05f, .95f, 1);
    for (int puff = 0; puff < 4; ++puff)
    {
        const float age = std::fmod(m_State->time * .22f + puff * .25f, 1.0f);
        const float drift = std::sin(age * 4 + position.x) * 9 + age * 18;
        m_Renderer.DrawEffectRect(screen.x + drift, screen.z + 60 + age * 90,
                                  22 + age * 42, 27 + age * 35, 0, .30f, .29f, .26f,
                                  std::sin(age * Pi) * .16f, 16);
    }
}

void WorldRenderer::DrawHerbPatch(Vec2 position) const
{
    const Vec2 screen = ToScreen(position);
    if (m_PropsTexture != 0)
    {
        DrawShadow(position, 22, 6);
        m_Renderer.DrawSprite(m_PropsTexture, screen.x, screen.z + (.5f - .21132453f) * 100,
                              100, 100, 1, 0, 2, 1);
        return;
    }
    m_Renderer.DrawSolidRect(screen.x, screen.z + 7, 42, 14, Pi * .25f, .12f, .24f, .09f, .38f);
    DrawBlock(m_Renderer, screen.x, screen.z + 14, 6, 23, .20f, .45f, .15f);
    DrawCanopy(m_Renderer, screen.x - 7, screen.z + 17, 22, 17, .28f, .68f, .22f);
    DrawCanopy(m_Renderer, screen.x + 6, screen.z + 24, 20, 18, .36f, .78f, .26f);
}

WorldRenderer::GroundChunk WorldRenderer::BuildGroundChunk(int chunkX, int chunkY) const
{
    // Fixed world-space lattice: adjacent chunks share exactly the same edges.
    const int minimumX = chunkX * GroundChunkDots;
    const int minimumY = chunkY * GroundChunkDots;
    GroundChunk chunk;
    const auto noise = [](int x, int y) {
        std::uint32_t value = static_cast<std::uint32_t>(x) * 374761393u +
                              static_cast<std::uint32_t>(y) * 668265263u;
        value = (value ^ (value >> 13)) * 1274126177u;
        return static_cast<float>((value ^ (value >> 16)) & 65535u) / 65535.0f;
    };
    for (int y = minimumY; y < minimumY + GroundChunkDots; ++y)
        for (int x = minimumX; x < minimumX + GroundChunkDots; ++x)
        {
            const float projectedX = (x + .5f) * GroundDotSize;
            const float projectedY = (y + .5f) * GroundDotSize;
            const Vec2 position = {projectedX / TileWidth + projectedY / TileHeight,
                                   projectedY / TileHeight - projectedX / TileWidth};
            const float grain = noise(x, y);
            const float patch = std::sin(position.x * .31f + std::sin(position.z * .19f)) * .5f +
                                std::cos(position.z * .37f) * .3f;
            const float edge = std::sin(position.x * 1.3f + position.z * .7f) * .16f +
                               std::sin(position.z * 2.1f - position.x * .5f) * .09f;
            const float road = RoadDistance(position);
            const float dirt = std::max(0.0f, std::min(1.0f, (.22f + edge - road) / .44f));
            const float shore = Distance(position, m_State->world.lake) - m_State->world.lakeRadius + edge;
            int color = std::max(0, std::min(4, static_cast<int>(2 + patch * 1.5f + (grain - .5f) * 2)));
            if (grain < dirt)
                color = 6 + std::max(0, std::min(5, static_cast<int>(2 + patch + grain * 3)));
            else if (grain > .92f && dirt < .25f)
                color = 5;
            if (shore < .65f)
            {
                const float wet = std::max(0.0f, std::min(1.0f, (.65f - shore) / .80f));
                if (grain < wet)
                    color = 12 + std::min(3, static_cast<int>(grain * 4));
            }
            if (shore < -.35f)
            {
                const float ripple = std::sin(projectedY * .14f + projectedX * .025f);
                color = 16 + std::max(0, std::min(4, static_cast<int>(1.5f + ripple + grain)));
                if (shore > -.70f && grain > .65f)
                    color = 21;
            }
            else if (dirt > .5f && grain > .975f)
                color = noise(x + 7, y - 3) > .5f ? 22 : 23;

            const float left = x * GroundDotSize;
            const float right = (x + 1) * GroundDotSize;
            const float bottom = y * GroundDotSize;
            const float top = (y + 1) * GroundDotSize;
            const float vertices[] = {left, bottom, 0, left, top, 0, right, top, 0,
                                      left, bottom, 0, right, top, 0, right, bottom, 0};
            chunk.batches[color].insert(chunk.batches[color].end(), std::begin(vertices), std::end(vertices));
        }
    return chunk;
}

void WorldRenderer::DrawGround() const
{
    if (m_State->width <= 0 || m_State->height <= 0)
        return;
    const Vec2 origin = ToScreen({0, 0});
    const float worldScale = WorldScale * m_State->height / 720.0f;
    const float halfWidth = m_State->width / (2.0f * worldScale);
    const float halfHeight = m_State->height / (2.0f * worldScale);
    const int centerX = static_cast<int>(std::floor(-origin.x / GroundChunkSize));
    const int centerY = static_cast<int>(std::floor(-origin.z / GroundChunkSize));
    const int radiusX = static_cast<int>(std::ceil(halfWidth / GroundChunkSize)) + 1;
    const int radiusY = static_cast<int>(std::ceil(halfHeight / GroundChunkSize)) + 1;
    const int minimumX = centerX - radiusX, maximumX = centerX + radiusX + 1;
    const int minimumY = centerY - radiusY, maximumY = centerY + radiusY + 1;
    auto &cache = m_GroundCache;
    if (cache.lake.x != m_State->world.lake.x || cache.lake.z != m_State->world.lake.z ||
        cache.lakeRadius != m_State->world.lakeRadius)
    {
        cache.chunks.clear();
        cache.ready = false;
        cache.lake = m_State->world.lake;
        cache.lakeRadius = m_State->world.lakeRadius;
    }
    if (!cache.ready || cache.minimumX != minimumX || cache.maximumX != maximumX ||
        cache.minimumY != minimumY || cache.maximumY != maximumY)
    {
        // Keep a bounded margin, evict distant chunks, and calculate only new chunks.
        for (auto it = cache.chunks.begin(); it != cache.chunks.end();)
        {
            const auto &key = it->first;
            if (key.first < minimumX || key.first >= maximumX || key.second < minimumY || key.second >= maximumY)
                it = cache.chunks.erase(it);
            else
                ++it;
        }
        for (int y = minimumY; y < maximumY; ++y)
            for (int x = minimumX; x < maximumX; ++x)
                if (cache.chunks.find({x, y}) == cache.chunks.end())
                    cache.chunks.emplace(std::make_pair(x, y), BuildGroundChunk(x, y));

        // One persistent GPU buffer; regroup/upload only when the chunk window changes.
        size_t total = 0;
        for (const auto &entry : cache.chunks)
            for (const auto &batch : entry.second.batches)
                total += batch.size();
        std::vector<float> vertices;
        vertices.reserve(total);
        for (size_t color = 0; color < cache.count.size(); ++color)
        {
            cache.first[color] = static_cast<int>(vertices.size() / 3);
            for (const auto &entry : cache.chunks)
            {
                const auto &batch = entry.second.batches[color];
                vertices.insert(vertices.end(), batch.begin(), batch.end());
            }
            cache.count[color] = static_cast<int>(vertices.size() / 3) - cache.first[color];
        }
        if (cache.buffer == 0)
            glGenBuffers(1, &cache.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, cache.buffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        cache.minimumX = minimumX;
        cache.maximumX = maximumX;
        cache.minimumY = minimumY;
        cache.maximumY = maximumY;
        cache.ready = true;
    }
    const float palette[][3] = {
        {.105f, .155f, .075f}, {.125f, .180f, .085f}, {.145f, .200f, .095f},
        {.165f, .220f, .105f}, {.185f, .240f, .115f}, {.225f, .270f, .135f},
        {.245f, .205f, .135f}, {.270f, .225f, .150f}, {.295f, .245f, .165f},
        {.320f, .265f, .180f}, {.345f, .290f, .200f}, {.370f, .315f, .220f},
        {.180f, .190f, .130f}, {.235f, .230f, .160f}, {.290f, .275f, .190f},
        {.340f, .320f, .230f}, {.060f, .165f, .180f}, {.070f, .190f, .205f},
        {.085f, .215f, .230f}, {.110f, .245f, .255f}, {.160f, .290f, .290f},
        {.260f, .355f, .330f}, {.295f, .300f, .235f}, {.400f, .385f, .295f}};

    // Reuse the solid-rectangle shader and submit one batch per palette color instead of per dot.
    m_Renderer.DrawSolidRect(0, 0, 1, 1, 0, palette[0][0], palette[0][1], palette[0][2], 1);
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    const GLint attribute = glGetAttribLocation(program, "a_Position");
    const GLint colorUniform = glGetUniformLocation(program, "u_Color");
    glUniform4f(glGetUniformLocation(program, "u_Trans"), origin.x / halfWidth, origin.z / halfHeight,
                1.0f / halfWidth, 1.0f / halfHeight);
    glUniform1i(glGetUniformLocation(program, "u_EffectType"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TerrainTexture);
    glUniform1i(glGetUniformLocation(program, "u_TerrainTexture"), 0);
    glUniform1i(glGetUniformLocation(program, "u_PixelTerrain"), m_PixelTerrain);
    glUniform2f(glGetUniformLocation(program, "u_TerrainTileSize"), TileWidth, TileHeight);
    std::array<Vec2, GameContent::MaxRoadHouses> houses = {}, bends = {};
    std::array<float, GameContent::MaxRoadHouses> junctions = {};
    for (size_t index = 0; index < Map().houses.size(); ++index)
    {
        houses[index] = Map().houses[index].position;
        bends[index] = Map().houses[index].roadBend;
        junctions[index] = Map().houses[index].junction;
    }
    const int houseCount = static_cast<int>(Map().houses.size());
    glUniform1i(glGetUniformLocation(program, "u_RoadHouseCount"), houseCount);
    glUniform2fv(glGetUniformLocation(program, "u_RoadHouses[0]"), houseCount, &houses[0].x);
    glUniform2fv(glGetUniformLocation(program, "u_RoadBends[0]"), houseCount, &bends[0].x);
    glUniform1fv(glGetUniformLocation(program, "u_RoadJunctions[0]"), houseCount, junctions.data());
    glUniform2f(glGetUniformLocation(program, "u_RoadPlaza"), Map().noticeBoard.x, Map().noticeBoard.z-2);
    std::array<Vec2, GameContent::MaxCampfires> fires = {};
    for (size_t index = 0; index < Map().campfires.size(); ++index)
        fires[index] = Map().campfires[index].position;
    const int fireCount = static_cast<int>(Map().campfires.size());
    glUniform1i(glGetUniformLocation(program, "u_CampfireCount"), fireCount);
    glUniform2fv(glGetUniformLocation(program, "u_Campfires[0]"), fireCount, &fires[0].x);
    std::array<std::array<float,4>, GameContent::MaxStoryRegions> regionBounds = {};
    std::array<std::array<float,3>, GameContent::MaxStoryRegions> regionColors = {};
    const auto &regions = GameContent::Get().story.regions;
    for (size_t index = 0; index < regions.size(); ++index)
    {
        const auto &region = regions[index];
        regionBounds[index] = {{region.minimumX,region.minimumZ,region.maximumX,region.maximumZ}};
        regionColors[index] = region.color;
    }
    const int regionCount = static_cast<int>(regions.size());
    glUniform1i(glGetUniformLocation(program,"u_RegionCount"),regionCount);
    glUniform4fv(glGetUniformLocation(program,"u_RegionBounds[0]"),regionCount,regionBounds[0].data());
    glUniform3fv(glGetUniformLocation(program,"u_RegionColors[0]"),regionCount,regionColors[0].data());
    glBindBuffer(GL_ARRAY_BUFFER, cache.buffer);
    glEnableVertexAttribArray(attribute);
    glVertexAttribPointer(attribute, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    for (size_t index = 0; index < cache.count.size(); ++index)
    {
        if (cache.count[index] == 0)
            continue;
        const bool texturedLand = m_TerrainTexture != 0 && (index < 16 || index >= 22);
        glUniform1i(glGetUniformLocation(program, "u_EffectType"), texturedLand ? 6 : 0);
        glUniform1i(glGetUniformLocation(program, "u_TerrainColumn"), index < 6 ? 0 : (index < 12 ? 1 : 2));
        glUniform4f(colorUniform, palette[index][0], palette[index][1], palette[index][2], 1);
        glDrawArrays(GL_TRIANGLES, cache.first[index], cache.count[index]);
    }
    glDisableVertexAttribArray(attribute);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUniform1i(glGetUniformLocation(program, "u_EffectType"), 0);
    // A world-space circle projects to an ellipse; animate its entire surface on the GPU.
    if (cache.lakeRadius > 0)
    {
        const Vec2 screen = ToScreen(cache.lake);
        const float width = cache.lakeRadius * TileWidth * std::sqrt(2.0f);
        const float height = cache.lakeRadius * TileHeight * std::sqrt(2.0f);
        if (std::abs(screen.x) <= halfWidth + width * .5f &&
            std::abs(screen.z) <= halfHeight + height * .5f)
            m_Renderer.DrawEffectRect(screen.x, screen.z, width, height, 0, .08f, .24f, .26f, .9f, 2);
    }
}

void WorldRenderer::RenderWorld(const GameState &state)
{
    m_Renderer.SetDamageFeedback(state.playerHitTime);
    m_Renderer.SetAttackFeedback(state.attackHitTime);
    m_State = &state;
    m_Renderer.SetWorldScale(WorldScale * state.height / 720.0f);
    DrawGround();
    enum class DrawType { House, Tree, Campfire, Npc, Animal, Tracks, Herb, Monster, Player, NoticeBoard, Decoration, Scenery, StoryMarker };
    struct DrawItem { Vec2 position; DrawType type; int index; };
    std::vector<DrawItem> items;
    for (int index = 0; index < static_cast<int>(Map().houses.size()); ++index)
        items.push_back({Map().houses[index].position, DrawType::House, index});
    items.push_back({Map().noticeBoard, DrawType::NoticeBoard, 0});
    for (int index = 0; index < static_cast<int>(WorldLayout::Trees().size()); ++index)
        if (Distance(WorldLayout::Trees()[index].position, state.world.lake) > state.world.lakeRadius + .5f)
            items.push_back({WorldLayout::Trees()[index].position, DrawType::Tree, index});
    for (int index = 0; index < static_cast<int>(WorldLayout::Landmarks().size()); ++index)
        if (Distance(WorldLayout::Landmarks()[index].position, state.world.lake) > state.world.lakeRadius + .5f)
            items.push_back({WorldLayout::Landmarks()[index].position, DrawType::Scenery, index});
    for (const auto &campfire : Map().campfires)
        items.push_back({campfire.position, DrawType::Campfire, 0});
    // Stable world cells: camera movement never changes an existing decoration's placement.
    const int centerX = static_cast<int>(std::floor(state.player.x / 1.4f));
    const int centerZ = static_cast<int>(std::floor(state.player.z / 1.4f));
    const int radius = static_cast<int>(std::ceil(10.0f + 5.0f * state.width / std::max(1.0f, static_cast<float>(state.height))));
    for (int z = centerZ - radius; z <= centerZ + radius; ++z)
        for (int x = centerX - radius; x <= centerX + radius; ++x)
        {
            std::uint32_t seed = static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(z) * 668265263u;
            seed = (seed ^ (seed >> 13)) * 1274126177u;
            const Vec2 p = {(x + .15f + (seed & 255u) / 365.0f) * 1.4f,
                            (z + .15f + ((seed >> 8) & 255u) / 365.0f) * 1.4f};
            const Vec2 screen = ToScreen(p);
            const float scale = WorldScale * state.height / 720.0f;
            if (std::abs(screen.x) * scale > state.width * .5f + 30 ||
                std::abs(screen.z) * scale > state.height * .5f + 35 ||
                Distance(p, state.world.lake) < state.world.lakeRadius + .35f)
                continue;
            bool occupied = false;
            for (const auto &house : Map().houses)
                occupied |= Distance(p, house.position) < 2.1f;
            for (const auto &fire : Map().campfires)
                occupied |= Distance(p, fire.position) < .7f;
            if (occupied)
                continue;
            const float road = RoadDistance(p);
            const float patch = std::sin(p.x * .65f + std::sin(p.z * .4f)) * std::cos(p.z * .53f);
            if (road > .15f && seed % 5 < 2 && patch > -.15f)
                items.push_back({p, DrawType::Decoration, static_cast<int>(seed % 10)});
            else if (road > -.35f && seed % 3 == 0)
                items.push_back({p, DrawType::Decoration, 8 + static_cast<int>(seed % 2)});
        }
    for (int index = 0; index < static_cast<int>(state.npcs.size()); ++index)
        items.push_back({state.npcs[index].position, DrawType::Npc, index});
    for (int index = 0; index < static_cast<int>(state.animals.size()); ++index)
        items.push_back({state.animals[index].position, DrawType::Animal, index});
    if (state.quest == QuestStage::Tracks)
        items.push_back({state.world.tracks, DrawType::Tracks, 0});
    if (state.quest == QuestStage::Herb)
        items.push_back({state.world.herb, DrawType::Herb, 0});
    if (state.quest == QuestStage::Hunt || state.quest == QuestStage::Trophy || StorySystem::IsHunting(state) || StorySystem::HasDrop(state))
        items.push_back({state.monster.position, DrawType::Monster, 0});
    if (StorySystem::Current(state) && !StorySystem::IsHunting(state) && !StorySystem::HasDrop(state))
        items.push_back({StorySystem::Target(state),DrawType::StoryMarker,0});
    items.push_back({state.player, DrawType::Player, 0});

    // Higher screen coordinates are farther away. Draw them first so foreground objects cover them.
    std::sort(items.begin(), items.end(), [this](const DrawItem &left, const DrawItem &right) {
        return ToScreen(left.position).z > ToScreen(right.position).z;
    });
    for (const DrawItem &item : items)
    {
        ApplyLocalLight(m_Renderer, item.position, state.time);
        switch (item.type)
        {
        case DrawType::StoryMarker:
        {
            const Vec2 p = ToScreen(item.position);
            m_Renderer.DrawEffectRect(p.x,p.z,28,13,0,.95f,.75f,.25f,.22f,3);
            m_Renderer.DrawSolidRect(p.x,p.z+15+std::sin(state.time*2)*2,6,6,Pi*.25f,.95f,.78f,.30f,.9f);
            break;
        }
        case DrawType::Scenery:
        {
            const auto &landmark = WorldLayout::Landmarks()[item.index];
            const Vec2 screen = ToScreen(item.position);
            DrawShadow(item.position, landmark.height * .45f, 10, landmark.height * .2f);
            if (landmark.kind == WorldLayout::SceneryKind::Boulder && m_RockTexture != 0)
            {
                DrawPixelSprite(item.position, m_RockTexture, landmark.height, landmark.height, .23f);
                break;
            }
            if (landmark.kind == WorldLayout::SceneryKind::Pine && m_PineTexture != 0)
            {
                DrawPixelSprite(item.position, m_PineTexture, landmark.height * (2.0f / 3.0f),
                                landmark.height, .0332f);
                break;
            }
            if (m_SceneryTexture != 0)
                m_Renderer.DrawSprite(m_SceneryTexture, screen.x, screen.z + (.5f-.27075773f)*landmark.height,
                                      landmark.height, landmark.height, static_cast<int>(landmark.kind), 0, 7, 1);
            else if (landmark.kind == WorldLayout::SceneryKind::Bush)
                DrawCanopy(m_Renderer, screen.x, screen.z + 10, 30, 26, .16f, .29f, .09f);
            else
                DrawBlock(m_Renderer, screen.x, screen.z + 10, 30, 22, .32f, .30f, .24f);
            break;
        }
        case DrawType::Decoration:
        {
            const Vec2 p = ToScreen(item.position);
            const float variation = .85f + item.index * .035f;
            if (item.index < 7)
                m_Renderer.DrawEffectRect(p.x, p.z + 6, 13 * variation, 17 * variation, 0,
                                          .22f * variation, .34f * variation, .13f, .9f, 14);
            else if (item.index == 7)
                m_Renderer.DrawEffectRect(p.x, p.z, 10, 4, .5f, .40f, .25f, .105f, .85f, 18);
            else
            {
                m_Renderer.DrawEffectRect(p.x + 1, p.z - 1, 12, 5, 0, .035f, .04f, .03f, .35f, 3);
                if (m_RockTexture != 0)
                    DrawPixelSprite(item.position, m_RockTexture, 20, 20, .23f);
                else
                    m_Renderer.DrawEffectRect(p.x, p.z + 1, 9, 6, -.2f, .38f, .37f, .30f, 1, 17);
            }
            break;
        }
        case DrawType::House: DrawHouse(item.position, .42f, .18f, .09f, item.index); break;
        case DrawType::Tree:
        {
            const auto &tree = WorldLayout::Trees()[item.index];
            if (tree.pine && m_PineTexture != 0)
            {
                const float height = 190.0f + tree.variant * 12;
                DrawShadow(item.position, height * .35f, 15, height * .7f);
                DrawPixelSprite(item.position, m_PineTexture, height * (2.0f / 3.0f), height, .0332f,
                                std::sin(state.time * 1.4f + item.position.x) * .02f);
            }
            else if (tree.pine && m_SceneryTexture != 0)
            {
                const Vec2 screen = ToScreen(item.position);
                const float height = 170.0f + tree.variant * 12;
                DrawShadow(item.position, height * .4f, 15, height * .7f);
                m_Renderer.DrawSprite(m_SceneryTexture, screen.x, screen.z + (.5f-.27075773f)*height,
                                      height, height, 6, 0, 7, 1, 1, 1, 1, 1,
                                      std::sin(state.time*1.4f + item.position.x)*.02f, .27075773f);
            }
            else
                DrawTree(item.position, tree.variant);
            break;
        }
        case DrawType::Campfire: DrawCampfire(item.position); break;
        case DrawType::Npc:
        {
            const NpcState &npc = state.npcs[item.index];
            const bool elder = Distance(npc.wanderSeed != 0 ? npc.home : npc.position, state.world.elder) < .1f;
            if (m_ElderTexture != 0 && elder)
            {
                DrawShadow(npc.position, 35, 9);
                DrawPixelSprite(npc.position, m_ElderTexture, 100 * (2.0f / 3.0f), 100, .0332f);
                break;
            }
            if (!elder && m_NpcTextures[item.index % 2] != 0)
            {
                DrawPixelNpc(npc, item.index % 2);
                break;
            }
            const Vec2 facing = npc.moving ? npc.facing : Vec2{state.player.x - npc.position.x, state.player.z - npc.position.z};
            const int frame = npc.moving ? (static_cast<int>(npc.phase) % 4 + 4) % 4 : 0;
            if (!DrawAtlasSprite(npc.position, m_HumanTexture, facing, frame, 100, HumanPivotY))
                DrawSprite(npc.position, npc.column, npc.row, 88, 112);
            break;
        }
        case DrawType::Animal:
        {
            const AnimalState &animal = state.animals[item.index];
            const Vec2 facing = {std::cos(animal.phase) * 2.4f,
                                  -std::sin(animal.phase * .7f) * 1.8f * .7f};
            const unsigned int texture = animal.column == 0 ? m_DeerTexture : (animal.column == 1 ? m_BoarTexture : m_SheepTexture);
            const int frame = static_cast<int>(state.time * 8.0f + animal.phase) % 4;
            if (!DrawPixelAnimal(animal.position, animal.column == 0 ? 0 : (animal.column == 1 ? 1 : 2), facing, frame,
                                 animal.column == 0 ? 88.0f : (animal.column == 1 ? 110.0f : 72.0f)) &&
                !DrawAtlasSprite(animal.position, texture, facing, frame, animal.column == 0 ? 128.0f : 110.0f, AnimalPivotY))
                DrawSprite(animal.position, animal.column, animal.row, 105, 82);
            break;
        }
        case DrawType::Tracks: DrawRect(item.position, 30, 9, Pi * .2f, .12f, .07f, .03f); break;
        case DrawType::Herb: DrawHerbPatch(item.position); break;
        case DrawType::Monster:
        {
            MonsterDrawContext context = {&m_Renderer, [this](Vec2 p) { return ToScreen(p); },
                                          [this, &state](Vec2 p, int c, int r, float w, float h, bool s) {
                                              const Vec2 facing = {state.player.x - p.x, state.player.z - p.z};
                                              const float distance = Distance(p, state.player);
                                              const bool walking = (state.quest == QuestStage::Hunt || StorySystem::IsHunting(state)) &&
                                                  distance > state.monster.definition.attackRange && distance < state.monster.definition.chaseRange;
                                              const bool chapterAnimation = state.monster.health <= 0 ||
                                                  state.monster.hitAnimation > 0 || state.monster.attackAnimation > 0;
                                              if ((!walking || chapterAnimation) && DrawChapter1Boar(state.monster)) return;
                                              const int frame = walking ? static_cast<int>(state.time * 8.0f) % 4 : 0;
                                              if (!DrawPixelAnimal(p, 1, facing, frame, state.monster.definition.spriteHeight, state.monster.health <= 0) &&
                                                  !DrawAtlasSprite(p, m_BoarTexture, facing, frame, state.monster.definition.spriteHeight, AnimalPivotY, state.monster.health <= 0))
                                                  DrawSprite(p, c, r, w*(state.monster.definition.spriteHeight/125), h*(state.monster.definition.spriteHeight/125), s);
                                          },
                                          state.width, state.height, state.time, Pi};
            m_Renderer.SetSpriteFlash(state.monster.health > 0 ? state.monster.hitFlash / .22f : 0);
            MonsterSystem::Draw(state.monster, context, state.quest == QuestStage::Trophy || StorySystem::HasDrop(state));
            m_Renderer.SetSpriteFlash(0);
            break;
        }
        case DrawType::Player:
        {
            const Vec2 foot = ToScreen(state.player);
            if (state.moving || state.dodge > 0)
                for (int puff = 0; puff < 3; ++puff)
                {
                    const float age = std::fmod(state.time * (state.sprint ? 2.8f : 1.9f) + puff / 3.0f, 1.0f);
                    const float backX = -(state.facing.x - state.facing.z) * age * 18;
                    const float backY = -(state.facing.x + state.facing.z) * age * 9;
                    m_Renderer.DrawEffectRect(foot.x + backX + (puff - 1) * 4, foot.z + backY + age * 5,
                                              9 + age * 15, 5 + age * 9, 0, .49f, .42f, .29f,
                                              std::sin(age * Pi) * .18f, 19);
                }
            const auto drawSwordSlash = [&]()
            {
                if (state.attack <= 0.0f || m_SwordSlashTexture == 0)
                    return;
                // pixelfx: 24 frames, 5 columns, 136x104 cells; origin is (44, 92) from top-left.
                const int frame = std::max(0, std::min(23, static_cast<int>((1.0f - state.attack / .32f) * 24)));
                const float angle = std::atan2((state.facing.x + state.facing.z) * .5f,
                                               state.facing.x - state.facing.z);
                constexpr float scale = .8f, offsetX = (68 - 44) * scale, offsetY = (92 - 52) * scale;
                const float c = std::cos(angle), s = std::sin(angle);
                m_Renderer.DrawSprite(m_SwordSlashTexture, foot.x + c * offsetX - s * offsetY,
                                      foot.z + 28 + s * offsetX + c * offsetY,
                                      136 * scale, 104 * scale, frame % 5, frame / 5, 5, 5,
                                      1, 1, 1, 1, 0, 0, angle);
            };
            if (m_SwordSlashTexture == 0 && state.attack > .035f && state.attack < .25f)
            {
                const float progress = 1.0f - state.attack / .32f;
                const float angle = std::atan2(state.facing.x + state.facing.z, state.facing.x - state.facing.z);
                const float swing = angle - Pi * .5f + (progress - .5f) * 1.8f;
                m_Renderer.DrawEffectRect(foot.x, foot.z + 28, 92, 55, swing,
                                          .76f, .85f, 1, std::sin(progress * Pi) * .48f, 15);
            }
            if (m_GeneralWalkTexture != 0)
            {
                m_Renderer.SetSpriteFlash(DamageFeedback::Fade(state.playerHitTime, DamageFeedback::FlashDuration));
                DrawGeneralPlayer(state);
                m_Renderer.SetSpriteFlash(0);
                drawSwordSlash();
                break;
            }
            if (m_PixelPlayer)
            {
                m_Renderer.SetSpriteFlash(DamageFeedback::Fade(state.playerHitTime, DamageFeedback::FlashDuration));
                DrawPixelPlayer(state);
                m_Renderer.SetSpriteFlash(0);
                drawSwordSlash();
                break;
            }
            if (state.attack > 0.0f && m_PlayerAttackTexture != 0)
            {
                const int frame = std::max(0, std::min(7, static_cast<int>((1.0f-state.attack/.32f)*8)));
                const int direction = (static_cast<int>(std::lround(std::atan2(-state.facing.z, -state.facing.x) / (Pi*.25f)))+8)%8;
                const Vec2 screen = ToScreen(state.player);
                // A wider attack canvas preserves the walking sprite's body scale and foot position.
                constexpr float height = 88.0f * (256.0f/192.0f), pivotY = .26802864f;
                DrawShadow(state.player, 88.0f*(128.0f/192.0f)*.6f, 9.0f);
                m_Renderer.SetSpriteFlash(DamageFeedback::Fade(state.playerHitTime, DamageFeedback::FlashDuration));
                m_Renderer.DrawSprite(m_PlayerAttackTexture, screen.x, screen.z+(.5f-pivotY)*height,
                                      height, height, direction, frame, 8, 8);
                m_Renderer.SetSpriteFlash(0);
                drawSwordSlash();
                break;
            }
            const float fps = state.sprint ? 8.0f : 6.0f;
            const int frame = state.moving ? static_cast<int>(state.time * fps) % 4 : 0;
            m_Renderer.SetSpriteFlash(DamageFeedback::Fade(state.playerHitTime, DamageFeedback::FlashDuration));
            if (!DrawAtlasSprite(state.player, m_PlayerTexture != 0 ? m_PlayerTexture : m_HumanTexture, state.facing, frame, 88, HumanPivotY))
                DrawSprite(state.player, 0, 0, 42, 54);
            m_Renderer.SetSpriteFlash(0);
            drawSwordSlash();
            break;
        }
        case DrawType::NoticeBoard:
        {
            const Vec2 screen = ToScreen(item.position);
            DrawShadow(item.position, 42, 9);
            if (m_NoticeBoardTexture != 0)
            {
                DrawPixelSprite(item.position, m_NoticeBoardTexture, 110, 110, .082f);
                break;
            }
            if (m_PropsTexture != 0)
            {
                m_Renderer.DrawSprite(m_PropsTexture, screen.x, screen.z + (.5f - .21132453f) * 110,
                                      110, 110, 0, 0, 2, 1);
                break;
            }
            for (int side = -1; side <= 1; side += 2)
            {
                DrawBlock(m_Renderer, screen.x + side * 16, screen.z + 18, 7, 42, .22f, .14f, .065f);
                m_Renderer.DrawSolidRect(screen.x + side * 16 - 1, screen.z + 18, 2, 42, 0, .29f, .20f, .09f, 1);
            }
            DrawBlock(m_Renderer, screen.x, screen.z + 30, 49, 32, .32f, .22f, .11f);
            m_Renderer.DrawSolidRect(screen.x, screen.z + 30, 38, 21, 0, .32f, .22f, .11f, 1);
            DrawCanopy(m_Renderer, screen.x, screen.z + 48, 57, 17, .30f, .17f, .075f);
            for (int note = -1; note <= 1; ++note)
            {
                m_Renderer.DrawSolidRect(screen.x + note * 11, screen.z + 30, 8, 13, 0, .73f, .65f, .43f, 1);
                m_Renderer.DrawSolidRect(screen.x + note * 11, screen.z + 32, 4, 2, 0, .35f, .28f, .16f, 1);
            }
            break;
        }
        }
    }
    if (state.playerHitTime > 0 && m_PlayerHitTexture != 0)
    {
        const float age = std::max(0.0f, DamageFeedback::Elapsed(state.playerHitTime) - DamageFeedback::HitStop);
        const int frame = std::min(27, static_cast<int>(age * 30));
        const Vec2 screen = ToScreen(state.playerHitPosition);
        m_Renderer.SetLocalLight(0, 0, 0);
        m_Renderer.DrawSprite(m_PlayerHitTexture, screen.x, screen.z + 28,
                              128, 88, frame % 5, frame / 5, 5, 6);
    }
    m_Renderer.SetLocalLight(0, 0, 0);
    for (const auto &number : state.damageNumbers)
    {
        const float age = .9f - number.remaining;
        const bool heavy = number.combo % 3 == 0;
        const int frames = heavy ? 28 : 16, columns = heavy ? 5 : 4, rows = heavy ? 6 : 4;
        const float width = heavy ? 216.0f : 136.0f, height = heavy ? 136.0f : 80.0f;
        const Vec2 screen = ToScreen(number.position);
        const int frame = std::min(frames - 1, static_cast<int>(age * 30));
        if (age <= (frames - 1) / 30.0f)
            m_Renderer.DrawSprite(m_AttackHitTextures[heavy ? 1 : 0],
                screen.x + (width * .5f - (heavy ? 84 : 44)) * 2,
                screen.z + 35 + ((heavy ? 106 : 64) - height * .5f) * 2,
                width * 2, height * 2, frame % columns, frame / columns, columns, rows);
        const int extraSparks = std::min(3, number.combo / 3);
        for (int extra = 0; extra < extraSparks; ++extra)
        {
            const float delayedAge = age - .035f * (extra + 1);
            if (delayedAge < 0 || delayedAge >= .5f) continue;
            const int sparkFrame = std::min(15, static_cast<int>(delayedAge * 30));
            const float angle = extra * 2.094f + number.combo * .65f;
            m_Renderer.DrawSprite(m_AttackHitTextures[0],
                screen.x + std::cos(angle) * 35 + 24,
                screen.z + 55 + std::sin(angle) * 20 + 24,
                136, 80, sparkFrame % 4, sparkFrame / 4, 4, 4, .85f);
        }
        if (number.combo >= 3)
        {
            const int goldFrame = std::min(30, static_cast<int>(age * 36));
            m_Renderer.DrawSprite(m_AttackHitTextures[2], screen.x, screen.z + 35 + 40 * 1.6f,
                320, 179.2f, goldFrame % 5, goldFrame / 5, 5, 7, .7f);
        }
        const std::wstring text = L"-" + std::to_wstring(number.damage);
        const float spread = static_cast<float>(number.combo % 3 - 1) * 12;
        m_Renderer.DrawLabel(state.width * .5f + screen.x + spread,
            state.height * .5f + screen.z + 80 + age * 35, text.c_str(),
            1, heavy ? .72f : .96f, .35f, heavy ? 1.5f : 1.2f,
            std::min(1.0f, number.remaining / .2f), true);
    }
    m_State = nullptr;
}
