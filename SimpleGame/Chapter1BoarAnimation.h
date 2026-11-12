#pragma once
#include <array>
#include "GameConstants.h"

namespace Chapter1BoarAnimation
{
enum Action { Attack, Hit, Death };
using GameConstants::Chapter1Boar::FrameMilliseconds;
constexpr float Duration(Action action)
{
    int total = 0;
    for (int milliseconds : FrameMilliseconds[action]) total += milliseconds;
    return total / 1000.0f;
}
inline int Frame(Action action, float elapsed)
{
    int end = 0;
    for (int frame = 0; frame < 6; ++frame)
    {
        end += FrameMilliseconds[action][frame];
        if (elapsed * 1000.0f < end) return frame;
    }
    return 5;
}
} // namespace Chapter1BoarAnimation
