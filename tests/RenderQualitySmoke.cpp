// Run from SimpleGame; output directory must already exist.
#include "stdafx.h"
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include "Renderer.h"
#include "WorldRenderer.h"
#include "GameManager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

void Check(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
    const std::filesystem::path output = argc > 1 ? argv[1] : ".";
    int glutArgc = 1;
    glutInit(&glutArgc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Render quality verification");
    glutHideWindow();
    Check(glewInit() == GLEW_OK, "GLEW initialization");
    while (glGetError() != GL_NO_ERROR) {}
    try
    {
        Renderer renderer(1280, 720);
        Check(renderer.IsInitialized(), "shader compilation or framebuffer creation");
        WorldRenderer world(renderer);
        GameState state = CreateInitialGameState();
        state.time = 1;
        const auto capture = [&](const wchar_t *name) {
            world.BeginFrame(state.time);
            world.RenderWorld(state);
            world.EndFrame(state.time);
            Check(glGetError() == GL_NO_ERROR, "world OpenGL error");
            Check(renderer.SaveScreenshot((output / name).wstring().c_str()), "screenshot failed");
        };
        capture(L"village.png");
        state.player = {-11, 4};
        capture(L"houses.png");
        state.player = {-1, 52};
        capture(L"forest.png");
        state.time = 2;
        capture(L"forest_wind.png");
        state.player = {43, 33};
        capture(L"lake.png");
        state.player = {state.world.herb.x, state.world.herb.z - 2};
        state.quest = QuestStage::Herb;
        capture(L"herbs.png");
        state.player = {0, -1.6f};
        state.monster.position = {0, 0};
        state.quest = QuestStage::Hunt;
        GameManager game(state);
        const int health = state.monster.health;
        game.HandleCommand(GameCommand::Attack);
        Check(state.monster.health < health, "attack no longer damages monster");
        game.BeginFrame(.12f);
        capture(L"combat.png");
        state.attack = 0;
        state.moving = true;
        state.sprint = true;
        capture(L"dust.png");

        // Diffuse white stays below the emission threshold; fire must exceed it.
        renderer.BeginFrame(1);
        renderer.SetWorldScale(1);
        renderer.DrawSolidRect(0, 0, 1280, 720, 0, 1, 1, 1, 1);
        float diffuse[4] = {};
        glReadPixels(640, 360, 1, 1, GL_RGBA, GL_FLOAT, diffuse);
        Check(*std::max_element(diffuse, diffuse + 3) <= .951f, "diffuse leaks into bloom");
        renderer.DrawEffectRect(0, 0, 200, 200, 0, 1, .35f, .05f, 1, 1);
        std::vector<float> fire(200 * 200 * 4);
        glReadPixels(540, 260, 200, 200, GL_RGBA, GL_FLOAT, fire.data());
        bool emission = false;
        for (size_t i = 0; i < fire.size(); i += 4)
            emission |= fire[i] > 1.05f;
        Check(emission, "HDR fire emission is missing or clipped");
        renderer.SetLocalLight(0, 0, 0);
        renderer.DrawSolidRect(0, 0, 20, 20, 0, .4f, .4f, .4f, 1);
        float unlit[4] = {}, lit[4] = {};
        glReadPixels(640, 360, 1, 1, GL_RGBA, GL_FLOAT, unlit);
        renderer.SetLocalLight(2, .5f, .1f);
        renderer.DrawSolidRect(0, 0, 20, 20, 0, .4f, .4f, .4f, 1);
        glReadPixels(640, 360, 1, 1, GL_RGBA, GL_FLOAT, lit);
        Check(lit[0] > unlit[0] * 2 && lit[0] > lit[1] && lit[1] > lit[2], "local warm lighting is missing");
        renderer.EndFrame(1);
        renderer.SetLocalLight(10, 0, 0);
        renderer.DrawSolidRect(0, 0, 20, 20, 0, .25f, .5f, .75f, 1);
        unsigned char ui[4] = {};
        glReadPixels(640, 360, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, ui);
        Check(std::abs(int(ui[0])-64) <= 1 && std::abs(int(ui[1])-128) <= 1 &&
              std::abs(int(ui[2])-191) <= 1, "world lighting affects HUD");
        renderer.Resize(640, 360);
        state.width = 640;
        state.height = 360;
        state.player = {0, -2.5f};
        capture(L"small.png");
        Check(glGetError() == GL_NO_ERROR, "resize OpenGL error");
        std::cout << "PASS: nine captures, damage, HDR emission, diffuse bloom exclusion, local warm lighting, HUD isolation and resize\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
