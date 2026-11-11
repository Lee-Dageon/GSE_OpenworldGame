// Run from SimpleGame so the existing shader paths resolve.
// Link against WorldRenderer, Renderer, ModelCache and Monster (not the game's main).
#include "stdafx.h"
#include "GlfwTestContext.h"
#include "WorldRenderer.h"
#include "Renderer.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace
{
PFNGLBUFFERDATAPROC originalBufferData;
int uploads = 0;
GLuint terrainBuffer = 0;
GLsizeiptr uploadedBytes = 0;
std::uint64_t uploadedHash = 0;

void Require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void GLAPIENTRY TrackBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    ++uploads;
    GLint buffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    terrainBuffer = static_cast<GLuint>(buffer);
    uploadedBytes = size;
    uploadedHash = 14695981039346656037ull;
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (GLsizeiptr i = 0; i < size; ++i)
        uploadedHash = (uploadedHash ^ bytes[i]) * 1099511628211ull;
    // Every dot remains on the same lattice, even across negative chunk coordinates.
    const auto *vertices = static_cast<const float *>(data);
    for (GLsizeiptr i = 0; i < size / sizeof(float); i += 3)
        Require(std::fmod(vertices[i], 2.0f) == 0 && std::fmod(vertices[i + 1], 2.0f) == 0,
                "terrain lattice seam");
    originalBufferData(target, size, data, usage);
}

void Render(WorldRenderer &world, const GameState &state)
{
    world.BeginFrame(state.time);
    world.RenderWorld(state);
    world.EndFrame(state.time);
    Require(glGetError() == GL_NO_ERROR, "OpenGL error");
}
}

int main(int argc, char **argv)
{
    GlfwTestContext context(1920, 1080, "Terrain cache smoke test");
    if (!context.IsReady())
        return 1;
    try
    {
        Renderer renderer(1920, 1080);
        Require(renderer.IsInitialized(), "renderer initialization failed");
        GameState state = CreateInitialGameState();
        state.width = 1920;
        state.height = 1080;
        originalBufferData = __glewBufferData;
        __glewBufferData = TrackBufferData;
        {
            WorldRenderer world(renderer);
            Render(world, state);
            Require(uploads == 1, "initial terrain must upload once");
            const GLuint initialBuffer = terrainBuffer;
            const GLsizeiptr initialBytes = uploadedBytes;
            const auto initialHash = uploadedHash;
            std::cout << "initial upload bytes: " << initialBytes << '\n';

            uploads = 0;
            const auto start = std::chrono::steady_clock::now();
            for (int frame = 0; frame < 120; ++frame)
            {
                state.time = frame / 60.0f;
                Render(world, state);
            }
            const double milliseconds = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count() / 120;
            Require(uploads == 0, "time/stationary frames must not upload terrain");
            std::cout << "stationary: 120 frames, 0 uploads, " << milliseconds << " ms/frame submission\n";

            state.player.x += .1f;
            Render(world, state);
            Require(uploads == 0, "small camera movement must reuse terrain");
            state.player.x += 12;
            Render(world, state);
            Require(uploads == 1 && terrainBuffer == initialBuffer && uploadedBytes == initialBytes,
                    "chunk crossing must update the same bounded buffer once");
            state.player = {0, -2.5f};
            Render(world, state);
            Require(uploadedHash == initialHash, "revisited terrain must be deterministic");

            uploads = 0;
            state.width = 640;
            state.height = 720;
            // Resize recreates the render target; only world rendering uploads terrain.
            renderer.Resize(state.width, state.height);
            Render(world, state);
            Require(uploads == 1 && uploadedBytes < initialBytes, "shrinking must evict distant chunks");
            state.width = 2560;
            state.height = 720;
            renderer.Resize(state.width, state.height);
            Render(world, state);
            Require(uploads == 2 && terrainBuffer == initialBuffer, "expansion must cover the viewport");

            uploads = 0;
            state.world.lake = {-30, -30};
            state.world.lakeRadius = 8;
            Render(world, state);
            Require(uploads == 1, "lake changes must invalidate terrain");
            for (Vec2 position : {Vec2{-60, -60}, Vec2{80, 80}, Vec2{-60, 80}, Vec2{80, -60}})
            {
                state.player = position;
                Render(world, state);
                Require(terrainBuffer == initialBuffer, "teleports must not allocate additional GPU buffers");
            }
            glFinish();
            std::cout << "PASS: small movement, chunk crossing, revisit, resize, lake edits, negative coordinates, teleports\n";
        }
        Require(glIsBuffer(terrainBuffer) == GL_FALSE, "terrain GPU buffer leaked");
        __glewBufferData = originalBufferData;
        std::cout << "PASS: GPU buffer released\n";
    }
    catch (const std::exception &error)
    {
        __glewBufferData = originalBufferData;
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
