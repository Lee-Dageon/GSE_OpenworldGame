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

#include <Windows.h>
#include <algorithm>
#include <cstdio>
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
const char *g_CapturePath = nullptr;
int g_CaptureFrames = 0;

void RenderScene()
{
    static int fpsStartTime = glutGet(GLUT_ELAPSED_TIME);
    static int frameCount = 0;

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
    uiState.attackTime = g_GameState.attack;
    uiState.dodgeTime = g_GameState.dodge;
    uiState.player = g_GameState.player;
    uiState.elder = g_GameState.world.elder;
    uiState.monster = g_GameState.monster.position;
    uiState.lake = g_GameState.world.lake;
    uiState.lakeRadius = g_GameState.world.lakeRadius;
    uiState.monsterVisible = g_GameState.quest == QuestStage::Hunt || g_GameState.quest == QuestStage::Trophy;
    uiState.hasObjectiveTarget = g_GameState.quest != QuestStage::Complete;
    switch (g_GameState.quest)
    {
    case QuestStage::Tracks: uiState.objectiveTarget = g_GameState.world.tracks; break;
    case QuestStage::Herb: uiState.objectiveTarget = g_GameState.world.herb; break;
    case QuestStage::Hunt:
    case QuestStage::Trophy: uiState.objectiveTarget = g_GameState.monster.position; break;
    default: uiState.objectiveTarget = g_GameState.world.elder; break;
    }
    uiState.objective = QuestSystem::GetObjective(g_GameState.quest);
    if (const wchar_t *hint = g_GameManager.GetHint())
        uiState.hint = hint;
    uiState.message = g_GameState.message;
    DrawGameUi(*g_Renderer, uiState);
    if (g_CapturePath && ++g_CaptureFrames == 5)
    {
        wchar_t filename[MAX_PATH];
        const bool pathValid = MultiByteToWideChar(CP_UTF8, 0, g_CapturePath, -1, filename, MAX_PATH) != 0;
        std::cout << (pathValid && g_Renderer->SaveScreenshot(filename) ? "GSE_CAPTURE_OK\n" : "GSE_CAPTURE_FAILED\n");
        glutLeaveMainLoop();
        return;
    }
    glutSwapBuffers();

    ++frameCount;
    const int now = glutGet(GLUT_ELAPSED_TIME);
    const int elapsed = now - fpsStartTime;
    if (elapsed >= 1000)
    {
        char title[80];
        std::snprintf(title, sizeof(title), "Greenwake Tutorial | FPS: %.1f", frameCount * 1000.0 / elapsed);
        glutSetWindowTitle(title);
        frameCount = 0;
        fpsStartTime = now;
    }
}

void UpdateFrame()
{
    // Poll Shift so pressing/releasing it while moving takes effect without another key event.
    DWORD foregroundProcess = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
    g_GameState.sprint = foregroundProcess == GetCurrentProcessId() && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
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

void MouseInput(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        const GameCommand command = HitTestGameUi(x, y, g_GameState.width, g_GameState.height);
        if (command != GameCommand::None)
        {
            g_GameManager.HandleCommand(command);
            return;
        }
        if (GameUiPanelContains(x, y, g_GameState.width, g_GameState.height))
            return;
    }
    g_GameManager.HandleCommand(GameInput::OnMouseButton(button == GLUT_LEFT_BUTTON, state == GLUT_DOWN));
}
} // namespace

int main(int argc, char **argv)
{
    for (int index = 1; index + 1 < argc; ++index)
        if (std::string(argv[index]) == "--capture")
        {
            g_CapturePath = argv[index + 1];
            for (int next = index; next + 2 < argc; ++next)
                argv[next] = argv[next + 2];
            argc -= 2;
            break;
        }
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
