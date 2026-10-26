#include "stdafx.h"
#include "WorldRenderer.h"

#include "Dependencies\\glew.h"
#include "Renderer.h"

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
const std::array<Vec2, 8> VillageHouses = {{{-14, 5}, {15, 5}, {-9, 19}, {10, -20},
                                         {-27, -14}, {27, -14}, {-25, 20}, {26, 19}}};
constexpr Vec2 NoticeBoardPosition = {2, 1.5f};
const std::array<float, 8> RoadJunctions = {{-1, 10, 15, -13, -22, -29, 26, 33}};
const std::array<Vec2, 8> RoadBends = {{{-8, 0}, {8, 10}, {-4, 18}, {6, -17},
                                     {-16, -21}, {16, -26}, {-14, 28}, {17, 32}}};

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
    const float lengthSquared = dx * dx + dz * dz;
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
    for (size_t index = 0; index < VillageHouses.size(); ++index)
    {
        const Vec2 start = {mainRoadX(RoadJunctions[index]), RoadJunctions[index]};
        const Vec2 door = {VillageHouses[index].x + .78f, VillageHouses[index].z - .28f};
        const float taper = std::max(0.0f, std::min(1.0f, Distance(position, door) / Distance(start, door)));
        const float width = .48f + .44f * taper;
        distance = std::min(distance, DistanceToSegment(position, start, RoadBends[index]) - width);
        distance = std::min(distance, DistanceToSegment(position, RoadBends[index], door) - width);
    }

    const float plazaX = (position.x - NoticeBoardPosition.x) / 3.4f;
    const float plazaZ = (position.z - (NoticeBoardPosition.z - 2.0f)) / 2.6f;
    const float plaza = (std::sqrt(plazaX * plazaX + plazaZ * plazaZ) - 1.0f) * 2.6f;
    return std::min(distance, plaza);
}
}

WorldRenderer::WorldRenderer(Renderer &renderer) : m_Renderer(renderer)
{
    m_TreeTexture = m_Renderer.LoadTexture(L"./Assets/Textures/Fantasy/tree.png");
    if (m_TreeTexture != 0)
        m_TreePivotY = .18508148f;
    else
        m_TreeTexture = m_Renderer.LoadTexture(L"./Assets/Textures/tree.png");
    m_TerrainTexture = LoadFantasyTexture(m_Renderer, L"terrain_atlas.png");
    m_HumanTexture = LoadFantasyTexture(m_Renderer, L"human_atlas.png");
    m_BoarTexture = LoadFantasyTexture(m_Renderer, L"boar_atlas.png");
    m_DeerTexture = LoadFantasyTexture(m_Renderer, L"deer_atlas.png");
    m_SheepTexture = LoadFantasyTexture(m_Renderer, L"sheep_atlas.png");
    m_HouseTexture = m_Renderer.LoadTexture(L"./Assets/Textures/Fantasy/house.png");
    m_PlayerTexture = m_Renderer.LoadTexture(L"./Assets/Hero/Textures/human_atlas.png");
    // Atlas cells share a texture: avoid mipmaps blending neighboring directions/tiles.
    for (unsigned int texture : {m_TerrainTexture, m_HumanTexture, m_PlayerTexture, m_BoarTexture, m_DeerTexture, m_SheepTexture})
        if (texture != 0)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    glBindTexture(GL_TEXTURE_2D, 0);
}

WorldRenderer::~WorldRenderer()
{
    for (unsigned int texture : {m_TreeTexture, m_HouseTexture, m_PlayerTexture, m_TerrainTexture, m_HumanTexture, m_BoarTexture, m_DeerTexture, m_SheepTexture})
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

void WorldRenderer::DrawShadow(Vec2 position, float width, float height) const
{
    const Vec2 screen = ToScreen(position);
    m_Renderer.DrawEffectRect(screen.x + 9, screen.z - 13, width, height, -.12f, .02f, .03f, .02f, .32f, 3);
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
    const float stride = player && m_State->moving ? std::sin(m_State->time * (m_State->sprint ? 13.0f : 9.0f)) : 0;
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

void WorldRenderer::DrawHouse(Vec2 position, float red, float green, float blue) const
{
    if (m_HouseTexture != 0)
    {
        const Vec2 screen = ToScreen(position);
        DrawShadow(position, 172, 36);
        m_Renderer.DrawSprite(m_HouseTexture, screen.x, screen.z + (.5f-.30755001f)*220,
                              220, 220, 0, 0, 1, 1);
        return;
    }
    const std::vector<float> &model = m_ModelCache.LoadOrCreate("house.v1", {158, 48, 88, 76, 103, 58, 23, 43});
    const Vec2 screen = ToScreen(position);
    DrawShadow(position, 172, 36);
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
        DrawShadow(position, spriteWidth * .65f, 17.0f);
        m_Renderer.DrawSprite(m_TreeTexture, screen.x, screen.z + (.5f - m_TreePivotY) * spriteHeight,
                              spriteWidth, spriteHeight, 0, 0, 1, 1, 1,
                              1.0f - variant * .025f, 1.0f, 1.0f - variant * .035f);
        return;
    }
    const std::vector<float> &model = m_ModelCache.LoadOrCreate("tree.v1", {48, 13, 10, 48, 48, 52});
    const float height = 48.0f + static_cast<float>(variant) * 10.0f;
    DrawShadow(position, model[0] * 1.4f, model[1] * 1.6f);
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
    for (int stone = -1; stone <= 1; ++stone)
        DrawBlock(m_Renderer, screen.x + stone * 18, screen.z - 2, 20, 14, .24f, .25f, .23f);
    DrawBlock(m_Renderer, screen.x, screen.z, 55, 17, .24f, .15f, .08f);
    // Keep flames upright; the shader forms the hot core and rising embers.
    m_Renderer.DrawEffectRect(screen.x, screen.z + 32, 48, 72, 0, 1, .35f, .05f, .95f, 1);
}

void WorldRenderer::DrawHerbPatch(Vec2 position) const
{
    const Vec2 screen = ToScreen(position);
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
    glUniform2f(glGetUniformLocation(program, "u_TerrainTileSize"), TileWidth, TileHeight);
    glUniform2fv(glGetUniformLocation(program, "u_RoadHouses[0]"), 8, &VillageHouses[0].x);
    glUniform2fv(glGetUniformLocation(program, "u_RoadBends[0]"), 8, &RoadBends[0].x);
    glUniform1fv(glGetUniformLocation(program, "u_RoadJunctions[0]"), 8, RoadJunctions.data());
    glUniform2f(glGetUniformLocation(program, "u_RoadPlaza"), NoticeBoardPosition.x, NoticeBoardPosition.z-2);
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
    m_State = &state;
    m_Renderer.SetWorldScale(WorldScale * state.height / 720.0f);
    DrawGround();
    enum class DrawType { House, Tree, Campfire, Npc, Animal, Tracks, Herb, Monster, Player, NoticeBoard };
    struct DrawItem { Vec2 position; DrawType type; int index; };
    std::vector<DrawItem> items;
    for (const Vec2 &house : VillageHouses)
        items.push_back({house, DrawType::House, 0});
    items.push_back({NoticeBoardPosition, DrawType::NoticeBoard, 0});
    for (int index = 0; index < 32; ++index)
        items.push_back({{4 + std::cos(index * 1.71f) * (43.0f + (index % 6) * 6.5f),
                         7 + std::sin(index * 1.71f) * (43.0f + (index % 6) * 6.5f)}, DrawType::Tree, index});
    const std::array<Vec2, 10> campfires = {
        {{-3, 0}, {8, 2}, {-9, -8}, {12, -12}, {-13, 12}, {16, 12}, {-24, 0}, {25, 1}, {0, 24}, {1, -25}}};
    for (const Vec2 &campfire : campfires)
        items.push_back({campfire, DrawType::Campfire, 0});
    for (int index = 0; index < static_cast<int>(state.npcs.size()); ++index)
        items.push_back({state.npcs[index].position, DrawType::Npc, index});
    for (int index = 0; index < static_cast<int>(state.animals.size()); ++index)
        items.push_back({state.animals[index].position, DrawType::Animal, index});
    if (state.quest == QuestStage::Tracks)
        items.push_back({state.world.tracks, DrawType::Tracks, 0});
    if (state.quest == QuestStage::Herb)
        items.push_back({state.world.herb, DrawType::Herb, 0});
    if (state.quest == QuestStage::Hunt || state.quest == QuestStage::Trophy)
        items.push_back({state.monster.position, DrawType::Monster, 0});
    items.push_back({state.player, DrawType::Player, 0});

    // Higher screen coordinates are farther away. Draw them first so foreground objects cover them.
    std::sort(items.begin(), items.end(), [this](const DrawItem &left, const DrawItem &right) {
        return ToScreen(left.position).z > ToScreen(right.position).z;
    });
    for (const DrawItem &item : items)
    {
        switch (item.type)
        {
        case DrawType::House: DrawHouse(item.position, .42f, .18f, .09f); break;
        case DrawType::Tree: DrawTree(item.position, item.index % 4); break;
        case DrawType::Campfire: DrawCampfire(item.position); break;
        case DrawType::Npc:
        {
            const NpcState &npc = state.npcs[item.index];
            const Vec2 facing = {state.player.x - npc.position.x, state.player.z - npc.position.z};
            if (!DrawAtlasSprite(npc.position, m_HumanTexture, facing, 0, 100, HumanPivotY))
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
            if (!DrawAtlasSprite(animal.position, texture, facing, frame, animal.column == 0 ? 128.0f : 110.0f, AnimalPivotY))
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
                                              const bool walking = state.quest == QuestStage::Hunt && distance > 1.1f && distance < 7.0f;
                                              const int frame = walking ? static_cast<int>(state.time * 8.0f) % 4 : 0;
                                              if (!DrawAtlasSprite(p, m_BoarTexture, facing, frame, 125, AnimalPivotY, state.monster.health <= 0))
                                                  DrawSprite(p, c, r, w, h, s);
                                          },
                                          state.width, state.height, state.time, Pi};
            MonsterSystem::Draw(state.monster, context, state.quest == QuestStage::Trophy);
            break;
        }
        case DrawType::Player:
        {
            const float fps = state.sprint ? 12.0f : 8.0f;
            const int frame = state.moving ? static_cast<int>(state.time * fps) % 4 : 0;
            if (!DrawAtlasSprite(state.player, m_PlayerTexture != 0 ? m_PlayerTexture : m_HumanTexture, state.facing, frame, 88, HumanPivotY))
                DrawSprite(state.player, 0, 0, 42, 54);
            break;
        }
        case DrawType::NoticeBoard:
        {
            const Vec2 screen = ToScreen(item.position);
            DrawShadow(item.position, 42, 9);
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
    m_State = nullptr;
}
