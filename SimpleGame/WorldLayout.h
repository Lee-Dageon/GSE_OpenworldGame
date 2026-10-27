#pragma once
#include "GameState.h"
#include <array>
#include <cmath>

// Rendering and movement share these fixed world placements.
namespace WorldLayout
{
const std::array<Vec2, 8> Houses = {{{-6, -1.7f}, {6, 4}, {-9, 19}, {10, -20},
                                   {-27, -14}, {27, -14}, {-25, 20}, {26, 19}}};
const std::array<Vec2, 40> Trees = {{{-3.7f, 2.8f}, {3.5f, -2.7f}, {4, 7}, {-8, 1},
    {8, 1}, {-6, 7}, {6, 9}, {-9, 8}, {11, 7}, {-10, -4}, {10, -6}, {-4, -9},
    {5, -11}, {-12, 12}, {14, 12}, {-15, 18}, {-5, 24}, {5, 26}, {16, 24},
    {-18, 28}, {-11, 32}, {-4, 34}, {6, 36}, {12, 33}, {20, 32}, {-24, 36},
    {-18, 41}, {-10, 44}, {-3, 45}, {5, 46}, {13, 44}, {22, 43}, {-20, 50},
    {-12, 53}, {-3, 56}, {7, 55}, {17, 54}, {28, 54}, {34, 48}, {31, 31}}};

enum class SceneryKind { Boulder, Bush, Log, Fence, Cart, Ruin, Pine };
struct Scenery
{
    Vec2 position;
    SceneryKind kind;
    float height;
    float radius;
};
const std::array<Scenery, 30> Landmarks = {{
    {{3.3f, 1}, SceneryKind::Boulder, 110, .65f}, {{-2.8f, -.5f}, SceneryKind::Bush, 80, 0},
    {{4, -1.2f}, SceneryKind::Cart, 115, .65f}, {{-4, 1.5f}, SceneryKind::Fence, 105, .45f},
    {{-4.8f, 2.1f}, SceneryKind::Fence, 105, .45f}, {{-2.8f, 4.9f}, SceneryKind::Log, 105, .4f},
    {{6.5f, 7.5f}, SceneryKind::Bush, 95, 0}, {{-7, 5}, SceneryKind::Bush, 95, 0},
    {{7, 6}, SceneryKind::Boulder, 130, .8f}, {{-10, 10}, SceneryKind::Log, 110, .45f},
    {{-12, 17}, SceneryKind::Fence, 105, .45f}, {{-13, 18}, SceneryKind::Fence, 105, .45f},
    {{12, -17}, SceneryKind::Cart, 115, .65f}, {{-8, 26}, SceneryKind::Boulder, 145, .9f},
    {{4, 27}, SceneryKind::Bush, 95, 0}, {{8, 31}, SceneryKind::Ruin, 150, .8f},
    {{10, 32}, SceneryKind::Boulder, 125, .75f}, {{-5, 37}, SceneryKind::Log, 125, .5f},
    {{-13, 39}, SceneryKind::Bush, 105, 0}, {{12, 42}, SceneryKind::Ruin, 165, .9f},
    {{14, 44}, SceneryKind::Ruin, 130, .7f}, {{-6, 50}, SceneryKind::Boulder, 160, 1.0f},
    {{3, 52}, SceneryKind::Log, 125, .5f}, {{33, 27}, SceneryKind::Boulder, 165, 1.0f},
    {{35, 28}, SceneryKind::Bush, 100, 0}, {{36, 30}, SceneryKind::Boulder, 150, .9f},
    {{48, 25}, SceneryKind::Ruin, 175, 1}, {{50, 26}, SceneryKind::Boulder, 150, .9f},
    {{56, 42}, SceneryKind::Bush, 110, 0}, {{57, 45}, SceneryKind::Boulder, 160, 1}
}};

inline bool IsBlocked(Vec2 p)
{
    for (const Vec2 &house : Houses)
        if (std::abs(p.x-house.x) < 1.55f && std::abs(p.z-house.z) < 1.4f)
            return true;
    for (const Vec2 &tree : Trees)
        if ((p.x-tree.x)*(p.x-tree.x) + (p.z-tree.z)*(p.z-tree.z) < .42f*.42f)
            return true;
    for (const Scenery &item : Landmarks)
        if (item.radius > 0 && (p.x-item.position.x)*(p.x-item.position.x) +
            (p.z-item.position.z)*(p.z-item.position.z) < (item.radius+.18f)*(item.radius+.18f))
            return true;
    return false;
}
}
