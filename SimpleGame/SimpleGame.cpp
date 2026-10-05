#include "stdafx.h"

#include "Dependencies\\glew.h"
#include "Dependencies\\freeglut.h"
#include "Actor.h"
#include "GameInput.h"
#include "GameManager.h"
#include "GameState.h"
#include "GameUI.h"
#include "QuestSystem.h"
#include "Renderer.h"
#include "WorldRenderer.h"

#include <algorithm>
#include <iostream>

namespace
{
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;

Renderer *g_Renderer = nullptr;
WorldRenderer *g_WorldRenderer = nullptr;
GameState g_GameState = CreateInitialGameState();
GameManager g_GameManager(g_GameState);
Actor g_SceneRoot("SceneRoot");

void RenderScene()
{
    g_WorldRenderer->BeginFrame(g_GameState.time);
    g_WorldRenderer->RenderWorld(g_GameState);
    g_WorldRenderer->EndFrame(g_GameState.time);

    GameUiState uiState;
    uiState.windowWidth = g_GameState.width;
    uiState.windowHeight = g_GameState.height;
    uiState.level = g_GameState.level;
    uiState.attackPower = g_GameState.attackPower;
    uiState.experience = g_GameState.experience;
    uiState.experienceToNextLevel = g_GameState.experienceToNextLevel;
    uiState.potions = g_GameState.potions;
    uiState.herbs = g_GameState.herbs;
    uiState.trophies = g_GameState.trophies;
    uiState.health = g_GameState.health;
    uiState.maxHealth = g_GameState.maxHealth;
    uiState.messageTime = g_GameState.messageTime;
    uiState.objective = QuestSystem::GetObjective(g_GameState.quest);
    if (const wchar_t *hint = g_GameManager.GetHint())
        uiState.hint = hint;
    uiState.message = g_GameState.message;
    DrawGameUi(*g_Renderer, uiState);
    glutSwapBuffers();
}

void UpdateFrame()
{
    const int now = glutGet(GLUT_ELAPSED_TIME);
    const float deltaTime = std::min(.05f, (now - g_GameState.lastTime) / 1000.0f);
    g_GameState.lastTime = now;
    g_GameManager.UpdateGame(deltaTime);
    g_GameManager.UpdateSceneGraph(g_SceneRoot, deltaTime);
    glutPostRedisplay();
}

void ResizeWindow(int width, int height)
{
    g_GameState.width = std::max(640, width);
    g_GameState.height = std::max(360, height);
    if (g_Renderer)
        g_Renderer->Resize(g_GameState.width, g_GameState.height);
}

void KeyDown(unsigned char key, int, int)
{
    const bool shiftHeld = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
    const GameCommand command = GameInput::OnKeyDown(g_GameState, key, shiftHeld);
    if (command == GameCommand::Quit)
        glutLeaveMainLoop();
    else
        g_GameManager.HandleCommand(command);
}

void KeyUp(unsigned char key, int, int)
{
    GameInput::OnKeyUp(g_GameState, key);
}

void MouseInput(int button, int state, int, int)
{
    g_GameManager.HandleCommand(GameInput::OnMouseButton(button == GLUT_LEFT_BUTTON, state == GLUT_DOWN));
}
} // namespace

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutCreateWindow("Greenwake Tutorial");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    if (glewInit() != GLEW_OK || !glewIsSupported("GL_VERSION_3_3"))
    {
        std::cerr << "OpenGL 3.3 initialization failed.\n";
        return 1;
    }

    g_Renderer = new Renderer(WindowWidth, WindowHeight);
    if (!g_Renderer->IsInitialized())
    {
        delete g_Renderer;
        return 1;
    }
    g_WorldRenderer = new WorldRenderer(*g_Renderer);
    g_GameState.lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutDisplayFunc(RenderScene);
    glutIdleFunc(UpdateFrame);
    glutReshapeFunc(ResizeWindow);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutMouseFunc(MouseInput);
    glutMainLoop();

    delete g_WorldRenderer;
    delete g_Renderer;
    return 0;
}
