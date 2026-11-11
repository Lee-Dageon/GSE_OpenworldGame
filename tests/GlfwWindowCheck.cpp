// Explicit runtime check. Creates a hidden window; does not capture images.
#include "GlfwTestContext.h"
#include "GameInput.h"
#include "Renderer.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace
{
void Require(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}
}

int main()
{
    GlfwTestContext context(1280, 720, "그린웨이크 입력·폰트 검사");
    if (!context.IsReady()) return 1;
    try
    {
        GLFWwindow *window = context.Window();
        Require(glfwGetWindowAttrib(window, GLFW_OPENGL_PROFILE) == GLFW_OPENGL_COMPAT_PROFILE,
                "renderer requires compatibility profile");
        GLboolean doubleBuffered = GL_FALSE;
        glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
        Require(doubleBuffered == GL_TRUE, "double buffering");
        Require(std::strcmp(glfwGetWindowTitle(window), "그린웨이크 입력·폰트 검사") == 0, "UTF-8 title");
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        Require(width > 0 && height > 0, "framebuffer size");
        Renderer renderer(width, height);
        Require(renderer.IsInitialized(), "renderer and Korean font initialization");
        renderer.BeginFrame(0);
        renderer.EndFrame(0);
        renderer.DrawLabel(20, 20, L"한글·WASD·가방·제작", 1, 1, 1);
        Require(glGetError() == GL_NO_ERROR, "Korean label upload/draw");
        glfwSwapBuffers(window);

        GameState state = CreateInitialGameState();
        Require(GameInput::OnKeyDown(state, GLFW_KEY_I, false) == GameCommand::ToggleInventory, "uppercase key mapping");
        Require(GameInput::OnKeyDown(state, GLFW_KEY_I, false) == GameCommand::None, "inventory repeat suppression");
        GameInput::OnKeyUp(state, GLFW_KEY_I);
        Require(!state.keys['i'], "key release normalization");
        GameInput::OnKeyDown(state, GLFW_KEY_W, true);
        Require(state.keys['w'] && state.sprint, "movement with Shift");
        Require(GameInput::OnKeyDown(state, 27, false) == GameCommand::Quit, "Escape ASCII mapping");
        Require(GameInput::OnMouseButton(true, true) == GameCommand::Attack, "left button attack");
        Require(GameInput::OnMouseButton(true, false) == GameCommand::None, "mouse release");
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        Require(glfwWindowShouldClose(window) == GLFW_TRUE, "close flag");
        std::cout << "PASS: GLFW context, UTF-8 title, font drawing, game input, close flag\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
