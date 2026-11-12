#include "stdafx.h"
#include "Renderer.h"

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

Renderer::Renderer(int windowSizeX, int windowSizeY)
{
    Initialize(windowSizeX, windowSizeY);
}

Renderer::~Renderer()
{
    for (const auto &entry : m_LabelCache)
    {
        glDeleteTextures(1, &entry.second.id);
    }
    DestroyRenderTarget();
    if (m_VBORect != 0)
    {
        glDeleteBuffers(1, &m_VBORect);
    }
    const GLuint programs[] = {m_SolidRectShader, m_SpriteShader, m_PostProcessShader};
    for (GLuint program : programs)
    {
        if (program != 0)
        {
            glDeleteProgram(program);
        }
    }
    if (m_GdiToken != 0)
    {
        m_Fonts.reset();
        Gdiplus::GdiplusShutdown(static_cast<ULONG_PTR>(m_GdiToken));
    }
}

void Renderer::Initialize(int windowSizeX, int windowSizeY)
{
    m_WindowSizeX = std::max(1, windowSizeX);
    m_WindowSizeY = std::max(1, windowSizeY);
    glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);

    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) == Gdiplus::Ok)
    {
        m_GdiToken = static_cast<std::uint64_t>(token);
    }
    if (m_GdiToken == 0)
        return;
    Gdiplus::FontFamily systemFont(m_FontFamilyName.c_str());
    if (systemFont.GetLastStatus() != Gdiplus::Ok)
    {
        m_Fonts = std::make_unique<Gdiplus::PrivateFontCollection>();
        if (m_Fonts->AddFontFile(L"./Fonts/NanumGothic-Regular.ttf") != Gdiplus::Ok)
        {
            std::cerr << "Cannot load Korean font: Fonts/NanumGothic-Regular.ttf\n";
            return;
        }
        Gdiplus::FontFamily fallback;
        int count = 0;
        WCHAR name[LF_FACESIZE] = {};
        if (m_Fonts->GetFamilies(1, &fallback, &count) != Gdiplus::Ok || count != 1 ||
            fallback.GetFamilyName(name) != Gdiplus::Ok)
        {
            std::cerr << "Cannot read bundled Korean font family.\n";
            return;
        }
        m_FontFamilyName = name;
    }

    m_SolidRectShader = CompileShaders("./Shaders/SolidRect.vs", "./Shaders/SolidRect.fs");
    m_SpriteShader = CompileShaders("./Shaders/Sprite.vs", "./Shaders/Sprite.fs");
    m_PostProcessShader = CompileShaders("./Shaders/PostProcess.vs", "./Shaders/PostProcess.fs");
    CreateVertexBufferObjects();
    const bool renderTargetReady = CreateRenderTarget();
    m_Initialized = m_GdiToken != 0 && m_SolidRectShader != 0 && m_SpriteShader != 0 && m_PostProcessShader != 0 &&
                    m_VBORect != 0 && renderTargetReady;
}

bool Renderer::IsInitialized() const
{
    return m_Initialized;
}

void Renderer::Resize(int windowSizeX, int windowSizeY)
{
    m_WindowSizeX = std::max(1, windowSizeX);
    m_WindowSizeY = std::max(1, windowSizeY);
    glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);
    if (m_Initialized)
    {
        DestroyRenderTarget();
        m_Initialized = CreateRenderTarget();
    }
}

void Renderer::BeginFrame(float time)
{
    m_Time = time;
    m_WorldLighting = true;
    SetLocalLight(0, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);
    glClearColor(0.10f, 0.15f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame(float time)
{
    m_WorldLighting = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_PostProcessShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_FrameColorTexture);
    glUniform1i(glGetUniformLocation(m_PostProcessShader, "u_Source"), 0);
    glUniform1f(glGetUniformLocation(m_PostProcessShader, "u_Time"), time);
    glUniform2f(glGetUniformLocation(m_PostProcessShader, "u_Resolution"), static_cast<float>(m_WindowSizeX),
                static_cast<float>(m_WindowSizeY));

    const GLint positionAttribute = glGetAttribLocation(m_PostProcessShader, "a_Position");
    glEnableVertexAttribArray(positionAttribute);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
    glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(positionAttribute);
    glEnable(GL_BLEND);
}

void Renderer::SetWorldScale(float scale)
{
    m_DrawScale = std::max(0.1f, scale);
}

void Renderer::SetLocalLight(float red, float green, float blue)
{
    m_LocalLight[0] = red;
    m_LocalLight[1] = green;
    m_LocalLight[2] = blue;
}

bool Renderer::SaveScreenshot(const wchar_t *filename)
{
    if (!m_Initialized)
        return false;
    std::vector<unsigned char> pixels(static_cast<size_t>(m_WindowSizeX) * m_WindowSizeY * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, m_WindowSizeX, m_WindowSizeY, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
    if (glGetError() != GL_NO_ERROR)
        return false;
    Gdiplus::Bitmap bitmap(m_WindowSizeX, m_WindowSizeY, m_WindowSizeX * 4,
                           PixelFormat32bppARGB, pixels.data());
    bitmap.RotateFlip(Gdiplus::RotateNoneFlipY);
    const CLSID pngEncoder = {0x557cf406, 0x1a04, 0x11d3, {0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
    return bitmap.Save(filename, &pngEncoder, nullptr) == Gdiplus::Ok;
}

void Renderer::CreateVertexBufferObjects()
{
    const float rect[] = {
        -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
    };
    glGenBuffers(1, &m_VBORect);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);
}

bool Renderer::CreateRenderTarget()
{
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    glGenTextures(1, &m_FrameColorTexture);
    glBindTexture(GL_TEXTURE_2D, m_FrameColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_WindowSizeX, m_WindowSizeY, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FrameColorTexture, 0);

    glGenRenderbuffers(1, &m_DepthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_WindowSizeX, m_WindowSizeY);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthRenderbuffer);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

void Renderer::DestroyRenderTarget()
{
    if (m_DepthRenderbuffer != 0)
    {
        glDeleteRenderbuffers(1, &m_DepthRenderbuffer);
        m_DepthRenderbuffer = 0;
    }
    if (m_FrameColorTexture != 0)
    {
        glDeleteTextures(1, &m_FrameColorTexture);
        m_FrameColorTexture = 0;
    }
    if (m_Framebuffer != 0)
    {
        glDeleteFramebuffers(1, &m_Framebuffer);
        m_Framebuffer = 0;
    }
}

bool Renderer::AddShader(GLuint shaderProgram, const char *shaderText, GLenum shaderType)
{
    const GLuint shaderObject = glCreateShader(shaderType);
    if (shaderObject == 0)
    {
        return false;
    }

    const GLchar *source[] = {shaderText};
    const size_t sourceLength = std::strlen(shaderText);
    if (sourceLength > INT_MAX)
    {
        glDeleteShader(shaderObject);
        return false;
    }
    const GLint lengths[] = {static_cast<GLint>(sourceLength)};
    glShaderSource(shaderObject, 1, source, lengths);
    glCompileShader(shaderObject);

    GLint success = GL_FALSE;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        GLchar infoLog[1024] = {};
        glGetShaderInfoLog(shaderObject, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compile failed: " << infoLog << '\n';
        glDeleteShader(shaderObject);
        return false;
    }
    glAttachShader(shaderProgram, shaderObject);
    glDeleteShader(shaderObject);
    return true;
}

bool Renderer::ReadFile(const char *filename, std::string *target)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << filename << " file loading failed.\n";
        return false;
    }
    std::string line;
    while (std::getline(file, line))
    {
        target->append(line);
        target->push_back('\n');
    }
    return true;
}

GLuint Renderer::CompileShaders(const char *filenameVS, const char *filenameFS)
{
    std::string vertexSource;
    std::string fragmentSource;
    if (!ReadFile(filenameVS, &vertexSource) || !ReadFile(filenameFS, &fragmentSource))
    {
        return 0;
    }

    const GLuint shaderProgram = glCreateProgram();
    if (shaderProgram == 0 || !AddShader(shaderProgram, vertexSource.c_str(), GL_VERTEX_SHADER) ||
        !AddShader(shaderProgram, fragmentSource.c_str(), GL_FRAGMENT_SHADER))
    {
        if (shaderProgram != 0)
        {
            glDeleteProgram(shaderProgram);
        }
        return 0;
    }

    glLinkProgram(shaderProgram);
    GLint success = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        GLchar infoLog[1024] = {};
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader link failed: " << infoLog << '\n';
        glDeleteProgram(shaderProgram);
        return 0;
    }
    return shaderProgram;
}

GLuint Renderer::LoadTexture(const wchar_t *filename)
{
    Gdiplus::Bitmap bitmap(filename);
    if (bitmap.GetLastStatus() != Gdiplus::Ok)
    {
        return 0;
    }

    Gdiplus::Rect area(0, 0, static_cast<INT>(bitmap.GetWidth()), static_cast<INT>(bitmap.GetHeight()));
    Gdiplus::BitmapData data = {};
    if (bitmap.LockBits(&area, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
    {
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, area.Width, area.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data.Scan0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    bitmap.UnlockBits(&data);
    return texture;
}

void Renderer::DrawSolidRect(float x, float y, float sizeX, float sizeY, float rotation, float r, float g, float b,
                             float a)
{
    const float normalizedX = x * 2.0f * m_DrawScale / static_cast<float>(m_WindowSizeX);
    const float normalizedY = y * 2.0f * m_DrawScale / static_cast<float>(m_WindowSizeY);
    const float normalizedSizeX = sizeX * m_DrawScale / static_cast<float>(m_WindowSizeX);
    const float normalizedSizeY = sizeY * m_DrawScale / static_cast<float>(m_WindowSizeY);

    glUseProgram(m_SolidRectShader);
    glUniform1i(glGetUniformLocation(m_SolidRectShader, "u_WorldLighting"), m_WorldLighting);
    glUniform3fv(glGetUniformLocation(m_SolidRectShader, "u_LocalLight"), 1, m_LocalLight);
    glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Trans"), normalizedX, normalizedY, normalizedSizeX,
                normalizedSizeY);
    glUniform1f(glGetUniformLocation(m_SolidRectShader, "u_Rotation"), rotation);
    glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Color"), r, g, b, a);

    const GLint positionAttribute = glGetAttribLocation(m_SolidRectShader, "a_Position");
    glEnableVertexAttribArray(positionAttribute);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
    glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(positionAttribute);
}

void Renderer::DrawEffectRect(float x, float y, float sizeX, float sizeY, float rotation, float r, float g, float b,
                              float a, int effectType)
{
    const float normalizedX = x * 2.0f * m_DrawScale / static_cast<float>(m_WindowSizeX);
    const float normalizedY = y * 2.0f * m_DrawScale / static_cast<float>(m_WindowSizeY);
    glUseProgram(m_SolidRectShader);
    glUniform1i(glGetUniformLocation(m_SolidRectShader, "u_WorldLighting"), m_WorldLighting);
    glUniform3fv(glGetUniformLocation(m_SolidRectShader, "u_LocalLight"), 1, m_LocalLight);
    glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Trans"), normalizedX, normalizedY,
                sizeX * m_DrawScale / static_cast<float>(m_WindowSizeX), sizeY * m_DrawScale / static_cast<float>(m_WindowSizeY));
    glUniform1f(glGetUniformLocation(m_SolidRectShader, "u_Rotation"), rotation);
    glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Color"), r, g, b, a);
    glUniform1f(glGetUniformLocation(m_SolidRectShader, "u_Time"), m_Time);
    glUniform1i(glGetUniformLocation(m_SolidRectShader, "u_EffectType"), effectType);
    const GLint positionAttribute = glGetAttribLocation(m_SolidRectShader, "a_Position");
    glEnableVertexAttribArray(positionAttribute);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
    glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(positionAttribute);
    glUniform1i(glGetUniformLocation(m_SolidRectShader, "u_EffectType"), 0);
}

void Renderer::DrawSprite(GLuint texture, float x, float y, float sizeX, float sizeY, int column, int row, int columns,
                            int rows, float alpha, float r, float g, float b, float sway, float swayPivot, float rotation,
                            float sourceWidth, float sourceHeight, float alphaCutoff)
{
    if (texture == 0)
    {
        return;
    }

    const float normalizedX = x * 2.0f * m_DrawScale / static_cast<float>(m_WindowSizeX);
    const float normalizedY = y * 2.0f * m_DrawScale / static_cast<float>(m_WindowSizeY);
    const float normalizedSizeX = sizeX * m_DrawScale / static_cast<float>(m_WindowSizeX);
    const float normalizedSizeY = sizeY * m_DrawScale / static_cast<float>(m_WindowSizeY);
    const float u0 = static_cast<float>(column) / columns;
    const float v0 = static_cast<float>(row) / rows;
    const float u1 = (column + sourceWidth) / columns;
    const float v1 = (row + sourceHeight) / rows;

    glUseProgram(m_SpriteShader);
    glUniform1i(glGetUniformLocation(m_SpriteShader, "u_WorldLighting"), m_WorldLighting);
    glUniform3fv(glGetUniformLocation(m_SpriteShader, "u_LocalLight"), 1, m_LocalLight);
    glUniform1f(glGetUniformLocation(m_SpriteShader, "u_Sway"), sway);
    glUniform1f(glGetUniformLocation(m_SpriteShader, "u_SwayPivot"), swayPivot);
    glUniform1f(glGetUniformLocation(m_SpriteShader, "u_Rotation"), rotation);
    glUniform1f(glGetUniformLocation(m_SpriteShader, "u_Aspect"),
                static_cast<float>(m_WindowSizeX) / static_cast<float>(m_WindowSizeY));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(m_SpriteShader, "u_Texture"), 0);
    glUniform4f(glGetUniformLocation(m_SpriteShader, "u_Trans"), normalizedX, normalizedY, normalizedSizeX,
                normalizedSizeY);
    glUniform4f(glGetUniformLocation(m_SpriteShader, "u_UvRect"), u0, v0, u1, v1);
    glUniform1f(glGetUniformLocation(m_SpriteShader, "u_Alpha"), alpha);
    glUniform1f(glGetUniformLocation(m_SpriteShader, "u_AlphaCutoff"), alphaCutoff);
    glUniform3f(glGetUniformLocation(m_SpriteShader, "u_Tint"), r, g, b);

    const GLint positionAttribute = glGetAttribLocation(m_SpriteShader, "a_Position");
    glEnableVertexAttribArray(positionAttribute);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
    glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(positionAttribute);
}

Renderer::LabelTexture Renderer::CreateLabelTexture(const wchar_t *text)
{
    LabelTexture result;
    Gdiplus::FontFamily family(m_FontFamilyName.c_str(), m_Fonts.get());
    Gdiplus::Font font(&family, 18.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Bitmap measuringBitmap(1, 1, PixelFormat32bppARGB);
    Gdiplus::Graphics measuringGraphics(&measuringBitmap);
    Gdiplus::RectF measured;
    measuringGraphics.MeasureString(text, -1, &font, Gdiplus::PointF(0.0f, 0.0f), &measured);
    result.width = std::max(8, static_cast<int>(std::ceil(measured.Width)) + 8);
    result.height = std::max(28, static_cast<int>(std::ceil(measured.Height)) + 8);

    Gdiplus::Bitmap bitmap(result.width, result.height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255, 255));
    graphics.DrawString(text, -1, &font, Gdiplus::PointF(2.0f, 1.0f), &brush);

    Gdiplus::Rect area(0, 0, result.width, result.height);
    Gdiplus::BitmapData data = {};
    if (bitmap.LockBits(&area, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
    {
        return {};
    }
    glGenTextures(1, &result.id);
    glBindTexture(GL_TEXTURE_2D, result.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, result.width, result.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data.Scan0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    bitmap.UnlockBits(&data);
    return result;
}

void Renderer::DrawLabel(float x, float y, const wchar_t *text, float r, float g, float b, float scale,
                         float alpha, bool centered)
{
    const std::wstring key(text);
    auto found = m_LabelCache.find(key);
    if (found == m_LabelCache.end())
    {
        found = m_LabelCache.emplace(key, CreateLabelTexture(text)).first;
    }
    const LabelTexture &label = found->second;
    if (label.id == 0)
    {
        return;
    }

    const float centerX = x + (centered ? 0.0f : label.width * scale * 0.5f) - m_WindowSizeX * 0.5f;
    const float centerY = y + label.height * scale * 0.5f - m_WindowSizeY * 0.5f;
    DrawSprite(label.id, centerX, centerY, label.width * scale, label.height * scale, 0, 0, 1,
               1, alpha, r, g, b);
}
