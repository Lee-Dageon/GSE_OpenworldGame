#pragma once

#include <Windows.h>
#include <GLFW/glfw3.h>

inline bool InitializeGlfwWindow(bool visible)
{
    // GLFW normally changes process DPI awareness. Keep the caller's existing
    // setting so Windows scaling does not change during the freeglut migration.
    const HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        const auto getAwareness = reinterpret_cast<DPI_AWARENESS_CONTEXT (WINAPI *)()>(
            GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
        const auto setAwareness = reinterpret_cast<BOOL (WINAPI *)(DPI_AWARENESS_CONTEXT)>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (getAwareness && setAwareness)
            setAwareness(getAwareness());
    }
    if (!glfwInit())
        return false;
    // The existing renderer uses the default VAO, requiring compatibility mode.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
    return true;
}
