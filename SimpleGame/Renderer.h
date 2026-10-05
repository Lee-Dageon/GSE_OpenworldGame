#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "Dependencies\\glew.h"

class Renderer
{
  public:
    Renderer(int windowSizeX, int windowSizeY);
    ~Renderer();

    bool IsInitialized() const;
    void Resize(int windowSizeX, int windowSizeY);
    void BeginFrame(float time);
    void EndFrame(float time);

    GLuint LoadTexture(const wchar_t *filename);
    void DrawSolidRect(float x, float y, float sizeX, float sizeY, float rotation, float r, float g, float b, float a);
    void DrawEffectRect(float x, float y, float sizeX, float sizeY, float rotation, float r, float g, float b, float a,
                        int effectType);
    void DrawSprite(GLuint texture, float x, float y, float sizeX, float sizeY, int column, int row, int columns,
                    int rows, float alpha = 1.0f, float r = 1.0f, float g = 1.0f, float b = 1.0f);
    void DrawLabel(float x, float y, const wchar_t *text, float r, float g, float b);

  private:
    struct LabelTexture
    {
        GLuint id = 0;
        int width = 0;
        int height = 0;
    };

    void Initialize(int windowSizeX, int windowSizeY);
    bool ReadFile(const char *filename, std::string *target);
    bool AddShader(GLuint shaderProgram, const char *shaderText, GLenum shaderType);
    GLuint CompileShaders(const char *filenameVS, const char *filenameFS);
    void CreateVertexBufferObjects();
    bool CreateRenderTarget();
    void DestroyRenderTarget();
    LabelTexture CreateLabelTexture(const wchar_t *text);
    void DrawTexturedQuad(GLuint shader, GLuint texture, float x, float y, float sizeX, float sizeY);

    bool m_Initialized = false;
    int m_WindowSizeX = 1;
    int m_WindowSizeY = 1;
    std::uint64_t m_GdiToken = 0;
    float m_Time = 0.0f;

    GLuint m_VBORect = 0;
    GLuint m_SolidRectShader = 0;
    GLuint m_SpriteShader = 0;
    GLuint m_PostProcessShader = 0;
    GLuint m_Framebuffer = 0;
    GLuint m_FrameColorTexture = 0;
    GLuint m_DepthRenderbuffer = 0;
    std::map<std::wstring, LabelTexture> m_LabelCache;
};
