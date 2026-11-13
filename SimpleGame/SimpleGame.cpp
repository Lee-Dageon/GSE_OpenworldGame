#include "stdafx.h"
#include "GameConstants.h"

#include <GL/glew.h>
#include "GlfwSetup.h"
#include "Actor.h"
#include "GameInput.h"
#include "GameManager.h"
#include "GameState.h"
#include "GameUI.h"
#include "QuestSystem.h"
#include "Renderer.h"
#include "WorldRenderer.h"
#include "CraftingSystem.h"
#include "GameContent.h"
#include "StorySystem.h"

#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>

namespace
{
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;

Renderer *g_Renderer = nullptr;
WorldRenderer *g_WorldRenderer = nullptr;
GameState g_GameState;
GameManager g_GameManager(g_GameState);
Actor g_SceneRoot("SceneRoot");
const char *g_CapturePath = nullptr;
std::string g_CaptureFilename;
int g_CaptureFrames = 0;
GLFWwindow *g_Window = nullptr;

int ElapsedMilliseconds()
{
    return static_cast<int>(glfwGetTime() * 1000.0);
}

void DrawChapterIntro()
{
    static std::string chapterId;
    static std::wstring chapterName, chapterTitle = L"튜토리얼";
    static float startedAt = g_GameState.time;
    const auto *step = StorySystem::Current(g_GameState);
    if (step && step->chapterId != chapterId)
    {
        for (const auto &chapter : GameContent::Get().story.chapters)
            if (chapter.id == step->chapterId)
            {
                chapterId = chapter.id;
                const auto separator = chapter.title.find(L':');
                chapterName = separator == std::wstring::npos ? L"" : chapter.title.substr(0, separator);
                chapterTitle = separator == std::wstring::npos ? chapter.title : chapter.title.substr(separator + 1);
                const auto first = chapterTitle.find_first_not_of(L" \t");
                chapterTitle = first == std::wstring::npos ? L"" : chapterTitle.substr(first);
                startedAt = g_GameState.time;
                break;
            }
    }
    const float elapsed = g_GameState.time - startedAt;
    if (elapsed < 0.0f || elapsed >= 3.5f || chapterTitle.empty())
        return;
    const float alpha = std::max(0.0f, std::min(1.0f, std::min(elapsed / .4f, 3.5f - elapsed)));
    const float scale = GameUiScale(g_GameState.width, g_GameState.height);
    const float centerX = g_GameState.width * .5f, centerY = g_GameState.height * .5f;
    g_Renderer->DrawSolidRect(0, 0, static_cast<float>(g_GameState.width), 150 * scale,
                              0, .025f, .03f, .025f, .72f * alpha);
    g_Renderer->DrawLabel(centerX, centerY + 18 * scale, chapterName.c_str(),
                          .9f, .72f, .4f, .9f * scale, alpha, true);
    g_Renderer->DrawLabel(centerX, centerY - 35 * scale, chapterTitle.c_str(),
                          1, .94f, .79f, 1.5f * scale, alpha, true);
}

void RenderScene()
{
    static int fpsStartTime = ElapsedMilliseconds();
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
    uiState.potions = g_GameState.inventory.Count(ItemId::Potion);
    uiState.herbs = g_GameState.inventory.Count(ItemId::Herb);
    uiState.trophies = g_GameState.inventory.Count(ItemId::BoarTusk);
    uiState.hides = g_GameState.inventory.Count(ItemId::BoarHide);
    uiState.itemCounts = g_GameState.inventory.stacks;
    uiState.inventoryOpen = g_GameState.inventoryOpen;
    uiState.hunterBladeCrafted = g_GameState.hunterBladeCrafted;
    uiState.hunterArmorCrafted = g_GameState.hunterArmorCrafted;
    uiState.armorRecipeUnlocked = g_GameState.armorRecipeUnlocked;
    uiState.defense = g_GameState.defense;
    uiState.locationName = StorySystem::RegionName(g_GameState.player);
    uiState.nearWorkbench = CraftingSystem::IsAtWorkbench(g_GameState);
    uiState.huntReported = g_GameState.quest == QuestStage::Complete;
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
    uiState.monsterVisible = g_GameState.quest == QuestStage::Hunt || g_GameState.quest == QuestStage::Trophy ||
        StorySystem::IsHunting(g_GameState) || StorySystem::HasDrop(g_GameState);
    uiState.hasObjectiveTarget = true;
    switch (g_GameState.quest)
    {
    case QuestStage::Tracks: uiState.objectiveTarget = g_GameState.world.tracks; break;
    case QuestStage::Herb: uiState.objectiveTarget = g_GameState.world.herb; break;
    case QuestStage::Hunt:
    case QuestStage::Trophy: uiState.objectiveTarget = g_GameState.monster.position; break;
    default: uiState.objectiveTarget = g_GameState.world.elder; break;
    }
    uiState.objective = QuestSystem::GetObjective(g_GameState.quest);
    if (g_GameState.quest == QuestStage::Complete && g_GameState.hunterBladeCrafted)
        uiState.objective = GameContent::Get().quest.repeatObjective;
    if (StorySystem::Current(g_GameState) || StorySystem::CanStart(g_GameState) ||
        (g_GameState.storyFinished && g_GameState.quest == QuestStage::Complete))
    {
        uiState.objective = StorySystem::Objective(g_GameState);
        uiState.objectiveTarget = StorySystem::Target(g_GameState);
    }
    if (const wchar_t *hint = g_GameManager.GetHint())
        uiState.hint = hint;
    uiState.message = g_GameState.message;
    DrawGameUi(*g_Renderer, uiState);
    if (g_GameState.combo > 1 && g_GameState.comboTime > 0)
    {
        const std::wstring comboText = std::to_wstring(g_GameState.combo) + L" COMBO";
        const float pop = std::max(0.0f, 1.0f - (GameConstants::Combo::Window - g_GameState.comboTime) / GameConstants::Combo::PopDuration);
        const float scale = GameUiScale(g_GameState.width, g_GameState.height) * GameConstants::Combo::TextScale * (1 + GameConstants::Combo::PopStrength * pop);
        const float alpha = std::min(1.0f, g_GameState.comboTime / GameConstants::Combo::FadeDuration);
        const float x = g_GameState.width * .5f, y = g_GameState.height * GameConstants::Combo::ScreenY + pop * GameConstants::Combo::PopRise;
        for (int trail = GameConstants::Combo::TrailCount; trail > 0; --trail)
            g_Renderer->DrawLabel(x - trail * GameConstants::Combo::TrailX * pop, y - trail * GameConstants::Combo::TrailY * pop, comboText.c_str(),
                1, .3f, .08f, scale, alpha * pop * GameConstants::Combo::TrailAlpha, true);
        for (int side : {-1, 1})
            g_Renderer->DrawLabel(x + side * GameConstants::Combo::OutlineOffset, y - GameConstants::Combo::OutlineOffset, comboText.c_str(), .15f, .04f, .02f, scale, alpha, true);
        g_Renderer->DrawLabel(x, y, comboText.c_str(), 1, .78f + .22f * pop, .3f + .7f * pop, scale, alpha, true);
    }
    DrawChapterIntro();
    if (g_CapturePath && ++g_CaptureFrames == 5)
    {
        wchar_t filename[MAX_PATH];
        const bool pathValid = MultiByteToWideChar(CP_UTF8, 0, g_CapturePath, -1, filename, MAX_PATH) != 0;
        std::cout << (pathValid && g_Renderer->SaveScreenshot(filename) ? "GSE_CAPTURE_OK\n" : "GSE_CAPTURE_FAILED\n");
        glfwSetWindowShouldClose(g_Window, GLFW_TRUE);
        return;
    }
    glfwSwapBuffers(g_Window);

    ++frameCount;
    const int now = ElapsedMilliseconds();
    const int elapsed = now - fpsStartTime;
    if (elapsed >= 1000)
    {
        wchar_t title[80];
        std::swprintf(title, 80, L"그린웨이크 | 초당 프레임: %.1f", frameCount * 1000.0 / elapsed);
        char utf8Title[256];
        if (WideCharToMultiByte(CP_UTF8, 0, title, -1, utf8Title, sizeof(utf8Title), nullptr, nullptr))
            glfwSetWindowTitle(g_Window, utf8Title);
        frameCount = 0;
        fpsStartTime = now;
    }
}

void UpdateFrame()
{
    // Restore held movement keys after focus changes or missed key events.
    const bool focused = glfwGetWindowAttrib(g_Window, GLFW_FOCUSED) == GLFW_TRUE;
    const auto down = [focused](int key) { return focused && (GetAsyncKeyState(key) & 0x8000) != 0; };
    g_GameState.keys['w'] = down('W') || down(VK_UP);
    g_GameState.keys['s'] = down('S') || down(VK_DOWN);
    g_GameState.keys['a'] = down('A') || down(VK_LEFT);
    g_GameState.keys['d'] = down('D') || down(VK_RIGHT);
    g_GameState.sprint = down(VK_SHIFT);
    const int now = ElapsedMilliseconds();
    const float deltaTime = std::min(GameConstants::Timing::MaxDeltaTime, (now - g_GameState.lastTime) / 1000.0f);
    g_GameState.lastTime = now;
    const float previousTime = g_GameState.time;
    g_GameManager.UpdateGame(deltaTime);
    g_GameManager.UpdateSceneGraph(g_SceneRoot, g_GameState.time - previousTime);
}

void ResizeWindow(GLFWwindow *, int width, int height)
{
    g_GameState.width = std::max(640, width);
    g_GameState.height = std::max(360, height);
    if (g_Renderer)
        g_Renderer->Resize(g_GameState.width, g_GameState.height);
}

void KeyInput(GLFWwindow *, int key, int, int action, int modifiers)
{
    // GameInput accepts ASCII; GLFW Escape is outside that range.
    if (key == GLFW_KEY_ESCAPE)
        key = 27;
    else if (key < GLFW_KEY_SPACE || key > GLFW_KEY_GRAVE_ACCENT)
        return;
    if (action == GLFW_RELEASE)
    {
        GameInput::OnKeyUp(g_GameState, static_cast<unsigned char>(key));
        return;
    }
    const GameCommand command = GameInput::OnKeyDown(g_GameState, static_cast<unsigned char>(key),
                                                    (modifiers & GLFW_MOD_SHIFT) != 0);
    if (command == GameCommand::Quit)
        glfwSetWindowShouldClose(g_Window, GLFW_TRUE);
    else
        g_GameManager.HandleCommand(command);
}

void FocusChanged(GLFWwindow *, int focused)
{
    if (!focused)
    {
        std::fill(std::begin(g_GameState.keys), std::end(g_GameState.keys), false);
        g_GameState.sprint = false;
    }
}

void MouseInput(GLFWwindow *window, int button, int action, int)
{
    double cursorX, cursorY;
    int width, height;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;
    const int x = static_cast<int>(cursorX * g_GameState.width / width);
    const int y = static_cast<int>(cursorY * g_GameState.height / height);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        const GameCommand command = HitTestGameUi(x, y, g_GameState.width, g_GameState.height, g_GameState.inventoryOpen);
        if (command != GameCommand::None)
        {
            g_GameManager.HandleCommand(command);
            return;
        }
        if (g_GameState.inventoryOpen || GameUiPanelContains(x, y, g_GameState.width, g_GameState.height))
            return;
    }
    g_GameManager.HandleCommand(GameInput::OnMouseButton(button == GLFW_MOUSE_BUTTON_LEFT, action == GLFW_PRESS));
}
} // namespace

int main(int argc, char **argv)
{
    std::filesystem::path launchDirectory;
    try
    {
        launchDirectory = std::filesystem::current_path();
        wchar_t executable[32768];
        const DWORD length = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));
        if (length == 0 || length >= std::size(executable))
            throw std::runtime_error("Cannot locate executable directory.");
        std::filesystem::current_path(std::filesystem::path(executable).parent_path());
        g_GameState = CreateInitialGameState();
    }
    catch (const std::exception &error)
    {
        std::cerr << "게임 데이터 로드 실패: " << error.what() << '\n';
        return 1;
    }
    bool debugEntrySelected = false;
    for (int index = 1; index < argc; ++index)
    {
        const std::string option = argv[index];
        if (option != "--capture" && option != "--debug-chapter" && option != "--debug-step")
        {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
        if (index + 1 >= argc)
        {
            std::cerr << "Missing value for " << option << '\n';
            return 1;
        }
        const std::string value = argv[++index];
        if (option == "--capture")
        {
            g_CaptureFilename = (launchDirectory / std::filesystem::u8path(value)).u8string();
            g_CapturePath = g_CaptureFilename.c_str();
        }
        else
        {
            if (debugEntrySelected)
            {
                std::cerr << "Choose only one debug chapter or step.\n";
                return 1;
            }
            const bool entered = option == "--debug-chapter"
                ? StorySystem::DebugEnterChapter(g_GameState, value)
                : StorySystem::DebugEnterStep(g_GameState, value);
            if (!entered)
            {
                std::cerr << "Cannot enter " << option << ' ' << value << '\n';
                return 1;
            }
            debugEntrySelected = true;
        }
    }
    glfwSetErrorCallback([](int code, const char *message) {
        std::cerr << "GLFW " << code << ": " << message << '\n';
    });
    if (!InitializeGlfwWindow(g_CapturePath == nullptr))
        return 1;
    g_Window = glfwCreateWindow(WindowWidth, WindowHeight, "그린웨이크 - 모험의 시작", nullptr, nullptr);
    if (!g_Window)
    {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_Window);
    // Preserve the previous path's driver-controlled swap interval.
    if (glewInit() != GLEW_OK || !glewIsSupported("GL_VERSION_3_3"))
    {
        std::cerr << "OpenGL 3.3 initialization failed.\n";
        glfwDestroyWindow(g_Window);
        glfwTerminate();
        return 1;
    }

    int result = 0;
    try
    {
        int width, height;
        glfwGetFramebufferSize(g_Window, &width, &height);
        ResizeWindow(g_Window, width, height);
        auto renderer = std::make_unique<Renderer>(g_GameState.width, g_GameState.height);
        if (!renderer->IsInitialized())
            throw std::runtime_error("Renderer initialization failed.");
        g_Renderer = renderer.get();
        auto world = std::make_unique<WorldRenderer>(*g_Renderer);
        g_WorldRenderer = world.get();
        g_GameState.lastTime = ElapsedMilliseconds();
        glfwSetFramebufferSizeCallback(g_Window, ResizeWindow);
        glfwSetKeyCallback(g_Window, KeyInput);
        glfwSetWindowFocusCallback(g_Window, FocusChanged);
        glfwSetMouseButtonCallback(g_Window, MouseInput);
        while (!glfwWindowShouldClose(g_Window))
        {
            glfwPollEvents();
            if (glfwWindowShouldClose(g_Window))
                break;
            glfwGetFramebufferSize(g_Window, &width, &height);
            if (width <= 0 || height <= 0)
            {
                glfwWaitEvents();
                continue;
            }
            UpdateFrame();
            RenderScene();
        }
        // Local owners release GPU resources before destroying the current context.
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    g_WorldRenderer = nullptr;
    g_Renderer = nullptr;
    glfwDestroyWindow(g_Window);
    g_Window = nullptr;
    glfwTerminate();
    return result;
}
