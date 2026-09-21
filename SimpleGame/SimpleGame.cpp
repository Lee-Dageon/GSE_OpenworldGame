#include "stdafx.h"
#include "Dependencies\\freeglut.h"
#include "Dependencies\\glew.h"
#include "Renderer.h"
#include "Monster.h"
#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cwchar>
#include <iostream>
#include <string>

namespace
{
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;
constexpr float Pi = 3.14159265f;
constexpr float TileWidth = 64.0f;
constexpr float TileHeight = 32.0f;
constexpr float WorldMin = -65.0f;
constexpr float WorldMax = 85.0f;

struct Npc
{
    Vec2 position;
    int column;
    int row;
    float phase;
};
struct Animal
{
    Vec2 position;
    Vec2 home;
    int column;
    int row;
    float phase;
    float speed;
};
enum class QuestStage
{
    Elder,
    Tracks,
    Herb,
    Hunt,
    Trophy,
    Return,
    Complete
};

Renderer *g_Renderer = nullptr;
int g_Width = WindowWidth;
int g_Height = WindowHeight;
std::array<bool, 256> g_Keys = {};
bool g_Sprint = false;
bool g_Moving = false;
Vec2 g_Player = {0.0f, -2.5f};
Vec2 g_Facing = {0.707f, 0.707f};
MonsterState g_Monster = MonsterSystem::CreateLakeBoar();
const Vec2 g_Elder = {0.0f, 1.4f};
const Vec2 g_Tracks = {13.0f, 13.0f};
const Vec2 g_Herb = {25.0f, 23.0f};
const Vec2 g_Lake = {45.0f, 39.0f};
constexpr float LakeRadius = 12.0f;
float g_Health = 100.0f;
float g_MaxHealth = 100.0f;
float g_Dodge = 0.0f;
float g_Attack = 0.0f;
float g_MessageTime = 7.0f;
float g_Time = 0.0f;
int g_Potions = 2;
int g_Herbs = 0;
int g_Trophies = 0;
int g_Level = 1;
int g_Experience = 0;
int g_ExperienceToNextLevel = 20;
int g_AttackPower = 2;
int g_LastTime = 0;
QuestStage g_Quest = QuestStage::Elder;
std::wstring g_Message = L"초록물결 마을에 오신 것을 환영합니다. 촌장 미라를 만나세요.";

std::array<Npc, 20> g_Npcs = {
    {{{0.0f, 1.4f}, 1, 0, 0}, {{-14, 1}, 2, 0, 1},   {{14, -2}, 3, 0, 2},   {{-20, 8}, 0, 1, 3},
     {{20, 8}, 0, 1, 4},      {{-7, -15}, 2, 1, 5},  {{7, -16}, 2, 1, 6},   {{-21, -7}, 1, 1, 7},
     {{21, -9}, 2, 1, 8},     {{4, 19}, 3, 1, 9},    {{-27, 4}, 3, 0, 10},  {{27, 4}, 0, 1, 11},
     {{-23, -19}, 1, 1, 12},  {{23, -19}, 2, 1, 13}, {{-11, 23}, 3, 1, 14}, {{12, 23}, 1, 1, 15},
     {{-31, -4}, 0, 1, 16},   {{31, -6}, 2, 1, 17},  {{-22, 21}, 3, 0, 18}, {{25, 19}, 0, 1, 19}}};

std::array<Animal, 8> g_Animals = {{{{-10, 8}, {-10, 8}, 0, 2, 0, 0.45f},
                                    {{-14, 14}, {-14, 14}, 2, 2, 1, 0.7f},
                                    {{20, 4}, {20, 4}, 1, 2, 2, 0.35f},
                                    {{24, 18}, {24, 18}, 0, 2, 3, 0.5f},
                                    {{5, 22}, {5, 22}, 2, 2, 4, 0.8f},
                                    {{30, 10}, {30, 10}, 1, 2, 5, 0.4f},
                                    {{-8, 28}, {-8, 28}, 0, 2, 6, 0.45f},
                                    {{28, 30}, {28, 30}, 2, 2, 7, 0.75f}}};

float Distance(Vec2 a, Vec2 b)
{
    const float x = a.x - b.x, z = a.z - b.z;
    return std::sqrt(x * x + z * z);
}
float DistanceToRoad(Vec2 p, Vec2 a, Vec2 b)
{
    const float dx = b.x - a.x, dz = b.z - a.z, lengthSquared = dx * dx + dz * dz;
    const float t =
        std::max(0.0f, std::min(1.0f, ((p.x - a.x) * dx + (p.z - a.z) * dz) / lengthSquared));
    return Distance(p, {a.x + dx * t, a.z + dz * t});
}
float RoadDistance(Vec2 p)
{
    float distance = DistanceToRoad(p, {0, -34}, {0, 34});
    distance = std::min(distance, DistanceToRoad(p, {-38, 0}, {38, 0}));
    distance = std::min(distance, DistanceToRoad(p, {-30, -26}, {30, 26}));
    distance = std::min(distance, DistanceToRoad(p, {-28, 25}, {28, -24}));
    distance = std::min(distance, DistanceToRoad(p, {0, 28}, {36, 34}));
    return distance;
}
Vec2 Screen(Vec2 p)
{
    const float x = p.x - g_Player.x, z = p.z - g_Player.z;
    return {(x - z) * TileWidth * 0.5f, (x + z) * TileHeight * 0.5f - 70.0f};
}
void Rect(Vec2 p, float w, float h, float rot, float r, float g, float b, float a = 1)
{
    const Vec2 s = Screen(p);
    g_Renderer->DrawSolidRect(s.x, s.z, w, h, rot, r, g, b, a);
}
void Message(const wchar_t *text)
{
    g_Message = text;
    g_MessageTime = 5.0f;
}

bool AddExperience(int amount)
{
    g_Experience += amount;
    bool leveledUp = false;

    while (g_Experience >= g_ExperienceToNextLevel)
    {
        g_Experience -= g_ExperienceToNextLevel;
        ++g_Level;
        ++g_AttackPower;
        g_MaxHealth += 15.0f;
        g_Health = g_MaxHealth;
        g_ExperienceToNextLevel += 10;
        leveledUp = true;
    }

    return leveledUp;
}

void SoftShadow(Vec2 p, float w, float h)
{
    const Vec2 s = Screen(p);
    g_Renderer->DrawSolidRect(
        s.x + 14, s.z - 16, w * 1.25f, h * 1.25f, -0.12f, 0.03f, 0.04f, 0.02f, 0.10f);
    g_Renderer->DrawSolidRect(s.x + 10, s.z - 14, w, h, -0.12f, 0.03f, 0.04f, 0.02f, 0.17f);
    g_Renderer->DrawSolidRect(
        s.x + 7, s.z - 12, w * 0.72f, h * 0.72f, -0.12f, 0.02f, 0.03f, 0.02f, 0.22f);
}
void Sprite(Vec2 p, int c, int r, float, float, bool shadow = true)
{
    const Vec2 s = Screen(p);
    if (r >= 2)
    {
        if (shadow)
        {
            SoftShadow(p, 36, 8);
        }
        const float furR = c == 0 ? .55f : (c == 1 ? .34f : .62f),
                    furG = c == 0 ? .34f : (c == 1 ? .22f : .43f), furB = c == 2 ? .25f : .12f;
        g_Renderer->DrawSolidRect(s.x, s.z + 8, 34, 17, 0, furR, furG, furB, 1);
        g_Renderer->DrawSolidRect(
            s.x + 19, s.z + 13, 15, 14, 0, furR * .9f, furG * .9f, furB * .9f, 1);
        for (int leg = -1; leg <= 1; leg += 2)
        {
            g_Renderer->DrawSolidRect(s.x + leg * 10, s.z - 3, 5, 14, 0, .18f, .12f, .07f, 1);
        }
        g_Renderer->DrawSolidRect(s.x + 23, s.z + 24, 4, 10, .3f, .75f, .61f, .39f, 1);
        return;
    }
    if (shadow)
    {
        SoftShadow(p, 25, 7);
    }
    const float colors[4][3] = {
        {.18f, .40f, .22f}, {.48f, .27f, .12f}, {.34f, .39f, .50f}, {.50f, .22f, .16f}};
    const float *cloth = colors[c % 4];
    const float bob = std::sin(g_Time * 8 + c) * 1.5f;
    g_Renderer->DrawSolidRect(s.x, s.z + 15 + bob, 18, 25, 0, cloth[0], cloth[1], cloth[2], 1);
    g_Renderer->DrawSolidRect(s.x, s.z + 34 + bob, 12, 12, 0, .80f, .62f, .42f, 1);
    g_Renderer->DrawSolidRect(s.x - 7, s.z - 1 + bob, 5, 14, 0, .16f, .11f, .07f, 1);
    g_Renderer->DrawSolidRect(s.x + 7, s.z - 1 + bob, 5, 14, 0, .16f, .11f, .07f, 1);
    g_Renderer->DrawSolidRect(
        s.x - 12, s.z + 17 + bob, 5, 18, 0, cloth[0] * .8f, cloth[1] * .8f, cloth[2] * .8f, 1);
    g_Renderer->DrawSolidRect(
        s.x + 12, s.z + 17 + bob, 5, 18, 0, cloth[0] * .8f, cloth[1] * .8f, cloth[2] * .8f, 1);
}
void House(Vec2 p, float roofR, float roofG, float roofB)
{
    const Vec2 s = Screen(p);
    SoftShadow(p, 155, 29);
    g_Renderer->DrawSolidRect(s.x, s.z - 13, 158, 48, Pi * .25f, .28f, .27f, .24f, 1);
    g_Renderer->DrawSolidRect(s.x - 31, s.z + 27, 88, 76, -.18f, .48f, .35f, .21f, 1);
    g_Renderer->DrawSolidRect(s.x + 31, s.z + 27, 88, 76, .18f, .38f, .27f, .17f, 1);
    for (int i = -1; i <= 1; ++i)
    {
        g_Renderer->DrawSolidRect(s.x - 31 + i * 27, s.z + 27, 4, 72, -.18f, .20f, .12f, .06f, .7f);
        g_Renderer->DrawSolidRect(s.x + 31 + i * 27, s.z + 27, 4, 72, .18f, .18f, .10f, .05f, .7f);
    }
    g_Renderer->DrawSolidRect(s.x - 28, s.z + 80, 103, 58, -.30f, roofR, roofG, roofB, 1);
    g_Renderer->DrawSolidRect(
        s.x + 29, s.z + 80, 103, 58, .30f, roofR * .72f, roofG * .72f, roofB * .72f, 1);
    g_Renderer->DrawSolidRect(s.x + 34, s.z + 8, 23, 43, .18f, .15f, .08f, .03f, 1);
    for (int side = -1; side <= 1; side += 2)
    {
        float wx = s.x + side * 35;
        g_Renderer->DrawSolidRect(wx, s.z + 35, 58, 48, 0, 1, .36f, .07f, .06f);
        g_Renderer->DrawSolidRect(wx, s.z + 35, 25, 24, 0, 1, .78f, .29f, .95f);
    }
    g_Renderer->DrawSolidRect(s.x + 35, s.z + 116, 18, 49, .08f, .24f, .20f, .17f, 1);
    float smoke = std::sin(g_Time * .8f + p.x) * 7;
    g_Renderer->DrawSolidRect(
        s.x + 42 + smoke, s.z + 151, 24, 24, Pi * .25f, .62f, .64f, .59f, .18f);
}
void Tree(Vec2 p, int variant)
{
    const Vec2 s = Screen(p);
    float height = 48 + variant * 10;
    SoftShadow(p, 48 + variant * 8, 13);
    g_Renderer->DrawSolidRect(
        s.x, s.z + height * .3f, 10 + variant * 2, height, 0, .27f, .15f, .07f, 1);
    if (variant == 0)
    {
        g_Renderer->DrawSolidRect(s.x, s.z + height, 48, 52, Pi * .25f, .16f, .40f, .17f, 1);
    }
    else if (variant == 1)
    {
        for (int i = 0; i < 3; ++i)
        {
            g_Renderer->DrawSolidRect(s.x,
                                      s.z + 42 + i * 18,
                                      55 - i * 12,
                                      38,
                                      Pi * .25f,
                                      .10f + i * .025f,
                                      .30f + i * .035f,
                                      .14f,
                                      1);
        }
    }
    else if (variant == 2)
    {
        g_Renderer->DrawSolidRect(s.x - 14, s.z + height, 42, 45, Pi * .25f, .28f, .43f, .16f, 1);
        g_Renderer->DrawSolidRect(
            s.x + 16, s.z + height + 8, 47, 42, Pi * .25f, .37f, .50f, .18f, 1);
    }
    else
    {
        g_Renderer->DrawSolidRect(
            s.x - 15, s.z + height - 2, 35, 36, Pi * .25f, .18f, .34f, .12f, 1);
        g_Renderer->DrawSolidRect(
            s.x + 14, s.z + height + 14, 31, 34, Pi * .25f, .22f, .39f, .13f, 1);
    }
}
void Campfire(Vec2 p)
{
    const Vec2 s = Screen(p);
    g_Renderer->DrawSolidRect(s.x, s.z + 18, 92, 74, 0, 1.0f, 0.36f, 0.08f, 0.08f);
    g_Renderer->DrawSolidRect(s.x, s.z, 55, 17, 0, 0.24f, 0.15f, 0.08f, 1);
    const float flicker = std::sin(g_Time * 12.0f) * 5.0f;
    g_Renderer->DrawSolidRect(
        s.x, s.z + 20, 24 + flicker, 42 + flicker, Pi * .25f, 1.0f, 0.35f, 0.05f, .8f);
    g_Renderer->DrawSolidRect(s.x, s.z + 25, 12, 30, Pi * .25f, 1.0f, 0.83f, 0.22f, .95f);
}
void HerbPatch(Vec2 p)
{
    const Vec2 s = Screen(p);
    const float pulse = .55f + std::sin(g_Time * 5.0f) * .16f;

    g_Renderer->DrawSolidRect(s.x, s.z + 7, 42, 14, Pi * .25f, .12f, .24f, .09f, .38f);
    g_Renderer->DrawSolidRect(s.x - 16, s.z + 15, 8, 30, -.35f, .20f, .66f, .24f, 1);
    g_Renderer->DrawSolidRect(s.x - 5, s.z + 20, 9, 38, -.15f, .28f, .82f, .28f, 1);
    g_Renderer->DrawSolidRect(s.x + 8, s.z + 18, 8, 34, .18f, .18f, .58f, .22f, 1);
    g_Renderer->DrawSolidRect(s.x + 18, s.z + 13, 7, 24, .42f, .34f, .75f, .26f, 1);
    g_Renderer->DrawSolidRect(s.x - 3, s.z + 42, 8, 8, Pi * .25f, .72f, .96f, .35f, pulse);
    g_Renderer->DrawSolidRect(s.x + 11, s.z + 34, 6, 6, Pi * .25f, .68f, .90f, .30f, pulse);
}
const wchar_t *Objective()
{
    switch (g_Quest)
    {
    case QuestStage::Elder:
        return L"촌장 미라와 대화하세요";
    case QuestStage::Tracks:
        return L"숲길에서 발자국을 조사하세요";
    case QuestStage::Herb:
        return L"호숫가의 회복 약초를 채집하세요";
    case QuestStage::Hunt:
        return L"호숫가를 떠도는 멧돼지를 사냥하세요";
    case QuestStage::Trophy:
        return L"멧돼지 엄니를 회수하세요";
    case QuestStage::Return:
        return L"촌장 미라에게 돌아가세요";
    case QuestStage::Complete:
        return L"퀘스트 완료 - 마을이 안전해졌습니다";
    }
    return L"";
}
void Interact()
{
    if (Distance(g_Player, g_Elder) < 1.5f)
    {
        if (g_Quest == QuestStage::Elder)
        {
            g_Quest = QuestStage::Tracks;
            Message(L"촌장 미라: 호수에서 온 이상한 발자국을 조사해 주세요.");
            return;
        }
        if (g_Quest == QuestStage::Return)
        {
            g_Quest = QuestStage::Complete;
            Message(L"퀘스트 완료! 개척 사냥꾼 외투를 받았습니다.");
            return;
        }
        Message(L"촌장 미라: 야생은 위험하지만 우리의 적은 아니랍니다.");
        return;
    }
    if (g_Quest == QuestStage::Tracks && Distance(g_Player, g_Tracks) < 1.3f)
    {
        g_Quest = QuestStage::Herb;
        Message(L"상처 입은 짐승의 발자국이 호수로 이어집니다.");
        return;
    }
    if (g_Quest == QuestStage::Herb && Distance(g_Player, g_Herb) < 1.3f)
    {
        ++g_Herbs;
        g_Quest = QuestStage::Hunt;
        Message(L"회복 약초를 획득했습니다. 호숫가에서 거친 울음이 들립니다.");
        return;
    }
    if (g_Quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(g_Monster, g_Player))
    {
        ++g_Trophies;
        g_Quest = QuestStage::Return;
        Message(L"엄니를 회수했습니다. 더 큰 짐승에게 입은 상처가 보입니다.");
        return;
    }
    for (const auto &n : g_Npcs)
    {
        if (Distance(g_Player, n.position) < 1.2f)
        {
            Message(L"마을 주민: 지난 폭풍 이후 숲의 짐승들이 불안해졌어요.");
            return;
        }
    }
}
void Attack()
{
    if (g_Attack > 0)
    {
        return;
    }
    g_Attack = .32f;
    if (g_Quest == QuestStage::Hunt)
    {
        const MonsterHitResult result = MonsterSystem::Hit(g_Monster, g_Player, g_AttackPower);

        if (result.defeated)
        {
            const bool leveledUp = AddExperience(25);
            g_Quest = QuestStage::Trophy;

            if (leveledUp)
            {
                Message(L"경험치 25 획득! 레벨이 올라 공격력과 최대 체력이 증가했습니다.");
            }
            else
            {
                Message(L"경험치 25 획득! 가까이 가서 E로 엄니를 회수하세요.");
            }
        }
    }
}
void Update(float dt)
{
    g_Time += dt;
    g_Dodge = std::max(0.f, g_Dodge - dt);
    g_Attack = std::max(0.f, g_Attack - dt);
    MonsterSystem::UpdateTimers(g_Monster, dt);
    g_MessageTime = std::max(0.f, g_MessageTime - dt);

    Vec2 m = {0, 0};
    if (g_Keys['w'])
    {
        m.x++;
        m.z++;
    }
    if (g_Keys['s'])
    {
        m.x--;
        m.z--;
    }
    if (g_Keys['a'])
    {
        m.x--;
        m.z++;
    }
    if (g_Keys['d'])
    {
        m.x++;
        m.z--;
    }
    const float length = std::sqrt(m.x * m.x + m.z * m.z);
    g_Moving = length > 0;
    if (length > 0)
    {
        m.x /= length;
        m.z /= length;
        g_Facing = m;
        float speed = g_Dodge > 0 ? 9.f : (g_Sprint ? 5.5f : 3.4f);
        Vec2 next = {g_Player.x + m.x * speed * dt, g_Player.z + m.z * speed * dt};
        next.x = std::max(WorldMin, std::min(WorldMax, next.x));
        next.z = std::max(WorldMin, std::min(WorldMax, next.z));
        if (Distance(next, g_Lake) > LakeRadius)
        {
            g_Player = next;
        }
    }
    for (auto &a : g_Animals)
    {
        a.phase += dt * a.speed;
        a.position.x = a.home.x + std::sin(a.phase) * 2.4f;
        a.position.z = a.home.z + std::cos(a.phase * .7f) * 1.8f;
    }
    if (g_Quest == QuestStage::Hunt)
    {
        const bool attacked = MonsterSystem::UpdateChaseAndAttack(g_Monster, g_Player, g_Dodge, dt, g_Health);

        if (attacked)
        {
            Message(L"멧돼지에게 공격받았습니다. Space로 회피하거나 Q로 회복하세요.");
        }
    }
    if (g_Health <= 0)
    {
        g_Player = {0, -2.5f};
        g_Health = g_MaxHealth;
        Message(L"구조대가 당신을 마을로 데려왔습니다.");
    }
}
void Ground()
{
    g_Renderer->DrawSolidRect(0, 0, (float)g_Width, (float)g_Height, 0, .18f, .32f, .22f, 1);
    int cx = (int)std::floor(g_Player.x), cz = (int)std::floor(g_Player.z);
    for (int z = cz - 14; z <= cz + 14; ++z)
    {
        for (int x = cx - 14; x <= cx + 14; ++x)
        {
            Vec2 p = {(float)x, (float)z};
            float noise = std::sin(x * 12.3f + z * 7.1f) * .025f, r = .24f + noise,
                  g = .47f + noise, b = .25f;
            const float road = RoadDistance(p);
            if (Distance(p, g_Lake) < LakeRadius + .5f)
            {
                r = .10f;
                g = .38f;
                b = .55f;
            }
            else if (road < 2.0f || Distance(p, {0, 0}) < 4.5f)
            {
                r = .39f;
                g = .34f;
                b = .23f;
                if (road < .58f)
                {
                    r = .50f;
                    g = .44f;
                    b = .30f;
                }
            }
            else if (Distance(p, {0, 0}) < 39)
            {
                r = .30f;
                g = .49f;
                b = .27f;
            }
            Rect(p, TileWidth * .72f, TileHeight * .72f, Pi * .25f, r, g, b, 1);
        }
    }
    for (int i = 0; i < 10; ++i)
    {
        float wave = std::fmod(g_Time * 18 + i * 31.f, 240.f);
        Rect({g_Lake.x + (i - 5) * 1.0f, g_Lake.z}, 35 + wave * .35f, 3, 0, .48f, .82f, .92f, .32f);
    }
}
void Scene()
{
    const std::array<Vec2, 16> houses = {{{-14, 5},
                                          {15, 5},
                                          {-9, 19},
                                          {10, -20},
                                          {-27, -14},
                                          {27, -14},
                                          {-25, 20},
                                          {26, 19},
                                          {-37, 7},
                                          {37, 6},
                                          {-20, -29},
                                          {21, -30},
                                          {-7, 32},
                                          {9, 33},
                                          {-34, -25},
                                          {35, 28}}};
    for (size_t i = 0; i < houses.size(); ++i)
    {
        House(houses[i], .42f + .04f * (i % 3), .18f, .09f);
    }
    for (int i = 0; i < 36; ++i)
    {
        float angle = i * 1.71f, radius = 43.0f + (i % 6) * 6.5f;
        Vec2 p = {4 + std::cos(angle) * radius, 7 + std::sin(angle) * radius};
        if (RoadDistance(p) > 5 && Distance(p, g_Lake) > LakeRadius + 5)
        {
            Tree(p, i % 4);
        }
    }
    const std::array<Vec2, 10> lights = {{{-3, 0},
                                          {8, 2},
                                          {-9, -8},
                                          {12, -12},
                                          {-13, 12},
                                          {16, 12},
                                          {-24, 0},
                                          {25, 1},
                                          {0, 24},
                                          {1, -25}}};
    for (const Vec2 &light : lights)
    {
        Campfire(light);
    }
    for (size_t i = 0; i < g_Npcs.size(); ++i)
    {
        Npc n = g_Npcs[i];
        if (i > 5)
        {
            n.position.x += std::sin(g_Time * .65f + n.phase) * .22f;
        }
        Sprite(n.position, n.column, n.row, 88, 112);
    }
    for (const auto &a : g_Animals)
    {
        int row = ((int)(g_Time * a.speed * 3) & 1) ? 3 : a.row;
        Sprite(a.position, a.column, row, 105, 82);
    }
    if (g_Quest == QuestStage::Tracks)
    {
        Rect(g_Tracks, 30, 9, Pi * .2f, .12f, .07f, .03f, 1);
    }
    if (g_Quest == QuestStage::Herb)
    {
        HerbPatch(g_Herb);
    }
    if (g_Quest == QuestStage::Hunt || g_Quest == QuestStage::Trophy)
    {
        MonsterDrawContext context = {g_Renderer, Screen, Sprite, g_Width, g_Height, g_Time, Pi};
        MonsterSystem::Draw(g_Monster, context, g_Quest == QuestStage::Trophy);
    }
    int playerColumn = (g_Moving && ((int)(g_Time * 7) & 1)) ? 3 : 0;
    Sprite(g_Player, playerColumn, 0, 42, 54);
    if (g_Attack > .12f)
    {
        Rect({g_Player.x + g_Facing.x * .8f, g_Player.z + g_Facing.z * .8f},
             62,
             9,
             -Pi * .2f,
             1,
             .78f,
             .25f,
             .8f);
    }
}
const wchar_t *Hint()
{
    if (Distance(g_Player, g_Elder) < 1.5f)
    {
        return L"E  촌장 미라와 대화";
    }
    if (g_Quest == QuestStage::Tracks && Distance(g_Player, g_Tracks) < 1.3f)
    {
        return L"E  발자국 조사";
    }
    if (g_Quest == QuestStage::Herb && Distance(g_Player, g_Herb) < 1.3f)
    {
        return L"E  약초 채집";
    }
    if (g_Quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(g_Monster, g_Player))
    {
        return L"E  엄니 회수";
    }
    for (const auto &n : g_Npcs)
    {
        if (Distance(g_Player, n.position) < 1.2f)
        {
            return L"E  대화";
        }
    }
    return nullptr;
}
void UI()
{
    float left = -g_Width * .5f, top = g_Height * .5f;
    g_Renderer->DrawSolidRect(left + 250, top - 66, 460, 104, 0, .025f, .04f, .03f, .88f);
    g_Renderer->DrawLabel(
        28, static_cast<float>(g_Height - 38), L"초록물결 마을 튜토리얼", .95f, .78f, .35f);
    g_Renderer->DrawLabel(28, static_cast<float>(g_Height - 70), Objective(), .95f, .94f, .83f);
    g_Renderer->DrawSolidRect(left + 135, top - 135, 220, 18, 0, .08f, .08f, .07f, .9f);
    const float healthRatio = g_Health / g_MaxHealth;
    g_Renderer->DrawSolidRect(left + 25 + healthRatio * 110.0f,
                              top - 135,
                              healthRatio * 220.0f,
                              12,
                              0,
                              .72f,
                              .16f,
                              .12f,
                              1);

    const float experienceRatio = static_cast<float>(g_Experience) / g_ExperienceToNextLevel;
    g_Renderer->DrawSolidRect(left + 135, top - 163, 220, 10, 0, .08f, .08f, .07f, .9f);
    g_Renderer->DrawSolidRect(left + 25 + experienceRatio * 110.0f,
                              top - 163,
                              experienceRatio * 220.0f,
                              7,
                              0,
                              .84f,
                              .68f,
                              .18f,
                              1);

    wchar_t status[160];
    std::swprintf(
        status,
        160,
        L"레벨 %d   체력 %.0f/%.0f   공격력 %d   경험치 %d/%d   회복약 %d   약초 %d   엄니 %d",
        g_Level,
        g_Health,
        g_MaxHealth,
        g_AttackPower,
        g_Experience,
        g_ExperienceToNextLevel,
        g_Potions,
        g_Herbs,
        g_Trophies);
    g_Renderer->DrawLabel(28, static_cast<float>(g_Height - 190), status, .94f, .94f, .84f);
    g_Renderer->DrawLabel(
        28,
        24,
        L"WASD 이동  |  Shift 달리기  |  E 상호작용  |  왼쪽 클릭 공격  |  Space 회피  |  Q 회복",
        .9f,
        .9f,
        .82f);
    if (const wchar_t *h = Hint())
    {
        g_Renderer->DrawSolidRect(0, -g_Height * .5f + 88, 390, 44, 0, .03f, .04f, .03f, .9f);
        g_Renderer->DrawLabel(g_Width * .5f - 150, 67, h, 1, .85f, .4f);
    }
    if (g_MessageTime > 0)
    {
        g_Renderer->DrawSolidRect(0, g_Height * .5f - 215, 850, 50, 0, .03f, .04f, .03f, .92f);
        g_Renderer->DrawLabel(std::max(20.f, g_Width * .5f - 405),
                              static_cast<float>(g_Height - 223),
                              g_Message.c_str(),
                              .98f,
                              .91f,
                              .7f);
    }
}
} // namespace

void RenderScene()
{
    g_Renderer->BeginFrame(g_Time);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    Ground();
    Scene();
    UI();
    g_Renderer->EndFrame(g_Time);
    glutSwapBuffers();
}
void Idle()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = std::min(.05f, (now - g_LastTime) / 1000.f);
    g_LastTime = now;
    Update(dt);
    glutPostRedisplay();
}
void ResizeWindow(int w, int h)
{
    g_Width = std::max(640, w);
    g_Height = std::max(360, h);
    if (g_Renderer)
    {
        g_Renderer->Resize(g_Width, g_Height);
    }
}
void KeyDown(unsigned char key, int, int)
{
    unsigned char k = (unsigned char)std::tolower(key);
    g_Keys[k] = true;
    g_Sprint = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
    if (k == 27)
    {
        glutLeaveMainLoop();
    }
    else if (k == 'e')
    {
        Interact();
    }
    else if (k == 'q' && g_Potions > 0 && g_Health < g_MaxHealth)
    {
        --g_Potions;
        g_Health = std::min(g_MaxHealth, g_Health + 45);
        Message(L"회복약을 사용했습니다.");
    }
    else if (k == ' ')
    {
        g_Dodge = .24f;
    }
}
void KeyUp(unsigned char key, int, int)
{
    g_Keys[(unsigned char)std::tolower(key)] = false;
    g_Sprint = false;
}
void MouseInput(int button, int state, int, int)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        Attack();
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutCreateWindow("Greenwake Tutorial");
    SetWindowTextW(GetForegroundWindow(), L"초록물결 - 쿼터뷰 RPG 튜토리얼");
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
    g_LastTime = glutGet(GLUT_ELAPSED_TIME);
    glutDisplayFunc(RenderScene);
    glutIdleFunc(Idle);
    glutReshapeFunc(ResizeWindow);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutMouseFunc(MouseInput);
    glutMainLoop();
    delete g_Renderer;
    return 0;
}
