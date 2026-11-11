#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <GL/glew.h>

namespace Gdiplus { class PrivateFontCollection; }

class Renderer
{
  public:
    Renderer(int windowSizeX, int windowSizeY);
    ~Renderer();

    bool IsInitialized() const;
    void Resize(int windowSizeX, int windowSizeY);
    void BeginFrame(float time);
    void EndFrame(float time);
    void SetWorldScale(float scale);
    void SetLocalLight(float red, float green, float blue);
    bool SaveScreenshot(const wchar_t *filename);

    GLuint LoadTexture(const wchar_t *filename);
    void DrawSolidRect(float x, float y, float sizeX, float sizeY, float rotation, float r, float g, float b, float a);
    void DrawEffectRect(float x, float y, float sizeX, float sizeY, float rotation, float r, float g, float b, float a,
                        int effectType);
    void DrawSprite(GLuint texture, float x, float y, float sizeX, float sizeY, int column, int row, int columns,
                    int rows, float alpha = 1.0f, float r = 1.0f, float g = 1.0f, float b = 1.0f,
                    float sway = 0.0f, float swayPivot = 0.0f, float rotation = 0.0f,
                    float sourceWidth = 1.0f, float sourceHeight = 1.0f, float alphaCutoff = .03f);
    void DrawLabel(float x, float y, const wchar_t *text, float r, float g, float b, float scale = 1.0f);

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
    float m_DrawScale = 1.0f;
    int m_WindowSizeX = 1;
    int m_WindowSizeY = 1;
    std::uint64_t m_GdiToken = 0;
    std::unique_ptr<Gdiplus::PrivateFontCollection> m_Fonts;
    std::wstring m_FontFamilyName = L"Malgun Gothic";
    float m_Time = 0.0f;
    bool m_WorldLighting = false;
    float m_LocalLight[3] = {};

    GLuint m_VBORect = 0;
    GLuint m_SolidRectShader = 0;
    GLuint m_SpriteShader = 0;
    GLuint m_PostProcessShader = 0;
    GLuint m_Framebuffer = 0;
    GLuint m_FrameColorTexture = 0;
    GLuint m_DepthRenderbuffer = 0;
    std::map<std::wstring, LabelTexture> m_LabelCache;
};
