#pragma once
#include <cmath>

namespace HorseSpriteFrames
{
inline constexpr int Width = 887, Height = 1774, Columns = 4, Rows = 8;
inline constexpr float Scale = .58f * .8f;
// Alpha >= 128 bounds, shared fixed hoof baseline for all four poses of each direction.
inline constexpr int Ground[Rows] = {223, 219, 216, 204, 209, 213, 207, 202};
inline constexpr int HorseTop[Rows] = {72, 71, 66, 56, 45, 47, 45, 52};
inline constexpr int RiderTop[Rows] = {20, 17, 16, 16, 11, 9, 12, 11};
inline constexpr int IdleTop[Rows] = {73, 66, 68, 65, 47, 42, 49, 59};
inline constexpr int IdleGround[Rows] = {216, 215, 214, 209, 213, 207, 206, 200};
inline constexpr float IdleFps = 3.0f;
// Screen direction order: SE, S, SW, W, NW, N, NE, E.
inline int Direction(float x, float z)
{
    constexpr float quarterPi = .785398163f;
    const int world = (static_cast<int>(std::lround(std::atan2(-z, -x) / quarterPi)) + 8) % 8;
    return (10 - world) % 8;
}
}
