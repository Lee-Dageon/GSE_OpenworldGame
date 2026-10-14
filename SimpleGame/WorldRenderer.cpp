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
constexpr float WorldScale = .72f;
constexpr float GroundDotSize = 6.0f;
constexpr int GroundChunkDots = 48;
constexpr float GroundChunkSize = GroundDotSize * GroundChunkDots;
const std::array<Vec2, 8> VillageHouses = {{{-14, 5}, {15, 5}, {-9, 19}, {10, -20},
                                         {-27, -14}, {27, -14}, {-25, 20}, {26, 19}}};
constexpr Vec2 NoticeBoardPosition = {2, 1.5f};

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
    const float junctions[] = {-1, 10, 15, -13, -22, -29, 26, 33};
    const Vec2 bends[] = {{-8, 0}, {8, 10}, {-4, 18}, {6, -17},
                          {-16, -21}, {16, -26}, {-14, 28}, {17, 32}};
    for (size_t index = 0; index < VillageHouses.size(); ++index)
    {
        const Vec2 start = {mainRoadX(junctions[index]), junctions[index]};
        const Vec2 door = {VillageHouses[index].x + .78f, VillageHouses[index].z - .28f};
        const float taper = std::max(0.0f, std::min(1.0f, Distance(position, door) / Distance(start, door)));
        const float width = .48f + .44f * taper;
        distance = std::min(distance, DistanceToSegment(position, start, bends[index]) - width);
        distance = std::min(distance, DistanceToSegment(position, bends[index], door) - width);
    }

    const float plazaX = (position.x - NoticeBoardPosition.x) / 3.4f;
    const float plazaZ = (position.z - (NoticeBoardPosition.z - 2.0f)) / 2.6f;
    const float plaza = (std::sqrt(plazaX * plazaX + plazaZ * plazaZ) - 1.0f) * 2.6f;
    return std::min(distance, plaza);
}
}

WorldRenderer::WorldRenderer(Renderer &renderer) : m_Renderer(renderer) {}

WorldRenderer::~WorldRenderer()
{
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
    m_Renderer.DrawEffectRect(screen.x + 9, screen.z - 13, width, height, -.12f, .02f, .03f, .02f, .32f, 3);
}

void WorldRenderer::DrawSprite(Vec2 position, int column, int row, float, float, bool shadow) const
{
    const Vec2 screen = ToScreen(position);
    if (shadow)
        DrawShadow(position, row >= 2 ? 54.0f : 38.0f, 9.0f);
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

    const float colors[4][3] = {{.12f, .28f, .16f}, {.48f, .16f, .08f}, {.10f, .24f, .42f}, {.36f, .12f, .30f}};
    const float *cloth = colors[column % 4];
    const float bob = std::sin(m_State->time * 5.0f + static_cast<float>(column)) * .8f;
    m_Renderer.DrawSolidRect(screen.x, screen.z + 18 + bob, 22, 29, 0, .025f, .018f, .012f, 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 18 + bob, 16, 24, 0, cloth[0], cloth[1], cloth[2], 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 13 + bob, 17, 3, 0, .24f, .13f, .055f, 1);
    m_Renderer.DrawSolidRect(screen.x - 12, screen.z + 20 + bob, 5, 16, 0, .035f, .025f, .018f, 1);
    m_Renderer.DrawSolidRect(screen.x + 12, screen.z + 20 + bob, 5, 16, 0, .035f, .025f, .018f, 1);
    m_Renderer.DrawSolidRect(screen.x - 12, screen.z + 20 + bob, 3, 13, 0, cloth[0] * .70f, cloth[1] * .70f,
                             cloth[2] * .70f, 1);
    m_Renderer.DrawSolidRect(screen.x + 12, screen.z + 20 + bob, 3, 13, 0, cloth[0] * .70f, cloth[1] * .70f,
                             cloth[2] * .70f, 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 36 + bob, 15, 15, 0, .08f, .045f, .022f, 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 36 + bob, 11, 11, 0, .74f, .52f, .32f, 1);
    m_Renderer.DrawSolidRect(screen.x, screen.z + 45 + bob, 19, 5, 0, .11f, .065f, .028f, 1);
    m_Renderer.DrawSolidRect(screen.x - 5, screen.z - 2 + bob, 6, 18, 0, .07f, .045f, .025f, 1);
    m_Renderer.DrawSolidRect(screen.x + 5, screen.z - 2 + bob, 6, 18, 0, .07f, .045f, .025f, 1);
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
    m_Renderer.DrawSolidRect(screen.x - 48, screen.z + 80, 12, 55, 0, .11f, .065f, .028f, 1);
    m_Renderer.DrawSolidRect(screen.x - 48, screen.z + 110, 17, 5, 0, .24f, .12f, .045f, 1);
    m_Renderer.DrawEffectRect(screen.x - 48, screen.z + 143, 14, 26, 0, 1.0f, .42f, .08f, .26f, 3);
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
    m_Renderer.DrawSolidRect(screen.x - 3, screen.z + height + 34, model[4] * .48f, model[5] * .46f, Pi * .25f,
                             .48f, .72f, .28f, .82f);
}

void WorldRenderer::DrawCampfire(Vec2 position) const
{
    const Vec2 screen = ToScreen(position);
    const float pulse = .82f + std::sin(m_State->time * 7.0f) * .08f;
    m_Renderer.DrawEffectRect(screen.x, screen.z + 17, 174, 118, Pi * .25f, 1.0f, .20f, .02f, .17f * pulse, 3);
    m_Renderer.DrawEffectRect(screen.x, screen.z + 20, 132, 98, Pi * .25f, 1.0f, .18f, .015f, .12f * pulse, 3);
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
    const float halfWidth = m_State->width / (2.0f * WorldScale);
    const float halfHeight = m_State->height / (2.0f * WorldScale);
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
    glBindBuffer(GL_ARRAY_BUFFER, cache.buffer);
    glEnableVertexAttribArray(attribute);
    glVertexAttribPointer(attribute, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    for (size_t index = 0; index < cache.count.size(); ++index)
    {
        if (cache.count[index] == 0)
            continue;
        glUniform4f(colorUniform, palette[index][0], palette[index][1], palette[index][2], 1);
        glDrawArrays(GL_TRIANGLES, cache.first[index], cache.count[index]);
    }
    glDisableVertexAttribArray(attribute);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Animate a few water highlights in the shader, not every terrain dot on the CPU.
    if (cache.lakeRadius > 0)
        for (int index = 0; index < 8; ++index)
        {
            const float angle = index * 2.4f;
            const Vec2 screen = ToScreen({cache.lake.x + std::cos(angle) * cache.lakeRadius * .5f,
                                          cache.lake.z + std::sin(angle) * cache.lakeRadius * .5f});
            const float width = std::min(48.0f, cache.lakeRadius * 8.0f);
            if (std::abs(screen.x) > halfWidth + width || std::abs(screen.z) > halfHeight + width)
                continue;
            m_Renderer.DrawEffectRect(screen.x, screen.z, width, width * .15f, 0, .26f, .46f, .43f, .20f, 2);
        }
}

void WorldRenderer::RenderWorld(const GameState &state)
{
    m_State = &state;
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
            DrawSprite(state.npcs[item.index].position, state.npcs[item.index].column, state.npcs[item.index].row, 88, 112);
            break;
        case DrawType::Animal:
            DrawSprite(state.animals[item.index].position, state.animals[item.index].column, state.animals[item.index].row, 105, 82);
            break;
        case DrawType::Tracks: DrawRect(item.position, 30, 9, Pi * .2f, .12f, .07f, .03f); break;
        case DrawType::Herb: DrawHerbPatch(item.position); break;
        case DrawType::Monster:
        {
            MonsterDrawContext context = {&m_Renderer, [this](Vec2 p) { return ToScreen(p); },
                                          [this](Vec2 p, int c, int r, float w, float h, bool s) { DrawSprite(p, c, r, w, h, s); },
                                          state.width, state.height, state.time, Pi};
            MonsterSystem::Draw(state.monster, context, state.quest == QuestStage::Trophy);
            break;
        }
        case DrawType::Player: DrawSprite(state.player, 0, 0, 42, 54); break;
        case DrawType::NoticeBoard:
        {
            const Vec2 screen = ToScreen(item.position);
            DrawShadow(item.position, 42, 9);
            for (int side = -1; side <= 1; side += 2)
            {
                m_Renderer.DrawSolidRect(screen.x + side * 16, screen.z + 18, 5, 42, 0, .16f, .10f, .045f, 1);
                m_Renderer.DrawSolidRect(screen.x + side * 16 - 1, screen.z + 18, 2, 42, 0, .29f, .20f, .09f, 1);
            }
            m_Renderer.DrawSolidRect(screen.x, screen.z + 30, 44, 27, 0, .12f, .075f, .03f, 1);
            m_Renderer.DrawSolidRect(screen.x, screen.z + 30, 38, 21, 0, .32f, .22f, .11f, 1);
            m_Renderer.DrawSolidRect(screen.x, screen.z + 46, 50, 5, 0, .25f, .14f, .065f, 1);
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
