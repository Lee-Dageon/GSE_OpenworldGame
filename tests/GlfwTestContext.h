#pragma once

#include <GL/glew.h>
#include "GlfwSetup.h"
#include <cstdio>

// Declared before Renderer so the context outlives every GPU resource in a test.
class GlfwTestContext
{
  public:
    GlfwTestContext(int width, int height, const char *title)
    {
        glfwSetErrorCallback([](int code, const char *message) {
            std::fprintf(stderr, "GLFW %d: %s\n", code, message);
        });
        m_Initialized = InitializeGlfwWindow(false);
        if (!m_Initialized)
            return;
        m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_Window)
            return;
        glfwMakeContextCurrent(m_Window);
        m_Ready = glewInit() == GLEW_OK && glewIsSupported("GL_VERSION_3_3");
        while (glGetError() != GL_NO_ERROR) {}
    }

    ~GlfwTestContext()
    {
        if (m_Window)
            glfwDestroyWindow(m_Window);
        if (m_Initialized)
            glfwTerminate();
    }

    GlfwTestContext(const GlfwTestContext &) = delete;
    GlfwTestContext &operator=(const GlfwTestContext &) = delete;
    bool IsReady() const { return m_Ready; }
    GLFWwindow *Window() const { return m_Window; }

  private:
    GLFWwindow *m_Window = nullptr;
    bool m_Initialized = false;
    bool m_Ready = false;
};
