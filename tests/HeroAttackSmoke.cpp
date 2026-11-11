// Run from SimpleGame; optionally pass a directory for actual renderer captures.
#include "stdafx.h"
#include "GlfwTestContext.h"
#include "GameManager.h"
#include "WorldRenderer.h"
#include "Renderer.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

void Require(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
    const std::filesystem::path output = argc > 1 ? argv[1] : "";
    GlfwTestContext context(1280, 720, "Hero attack verification");
    if (!context.IsReady()) return 1;
    try
    {
        Renderer renderer(1280, 720);
        Require(renderer.IsInitialized(), "renderer initialization");
        WorldRenderer world(renderer);
        GameState state = CreateInitialGameState();
        GameManager game(state);
        const auto render = [&](int capture) {
            world.BeginFrame(state.time);
            world.RenderWorld(state);
            world.EndFrame(state.time);
            Require(glGetError() == GL_NO_ERROR, "OpenGL error");
            if (!output.empty() && capture >= 0)
                Require(renderer.SaveScreenshot((output / ("attack_"+std::to_string(capture)+".png")).wstring().c_str()), "capture failed");
        };
        for (int direction = 0; direction < 8; ++direction)
        {
            const float angle = direction * 3.14159265f / 4;
            state.facing = {-std::cos(angle), -std::sin(angle)};
            game.HandleCommand(GameCommand::Attack);
            Require(std::abs(state.attack-.32f) < .00001f, "attack did not start");
            game.BeginFrame(.02f);
            for (int frame = 0; frame < 8; ++frame)
            {
                Require(state.attack > 0, "attack ended early");
                const float remaining = state.attack;
                game.HandleCommand(GameCommand::Attack);
                Require(state.attack == remaining, "repeated input restarted attack");
                render(direction == 0 ? frame : -1);
                game.BeginFrame(.04f);
            }
            Require(state.attack == 0, "attack timer did not end");
            render(direction == 0 ? 8 : -1);
        }
        state.facing = {-1, 0};
        state.moving = true;
        state.time = .125f;
        render(9);
        std::cout << "PASS: attack command, repeat guard, eight directions, timer completion, idle/walk recovery and OpenGL rendering\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
