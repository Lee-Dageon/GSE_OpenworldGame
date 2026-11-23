#pragma once
#include "GameConstants.h"
#include <algorithm>

namespace WeaponMotion
{
inline float Duration(int style)
{
    return style == 2 ? .65f : style == 1 ? .48f : GameConstants::Player::AttackDuration;
}
inline float Windup(int style) { return style == 2 ? .25f : style == 1 ? .16f : 0.0f; }
inline float Progress(int style, float remaining)
{
    return std::clamp(1.0f - remaining / Duration(style), 0.0f, 1.0f);
}
// One release per attack, including updates that cross the entire animation.
inline bool Release(int style, float remaining, bool &pending)
{
    if (!pending || remaining > Duration(style) - Windup(style)) return false;
    pending = false;
    return true;
}
inline float Reach(int style, float remaining)
{
    const float elapsed = Duration(style) - std::clamp(remaining, 0.0f, Duration(style));
    const float windup = Windup(style);
    if (windup > 0 && elapsed < windup * .65f) return -elapsed / (windup * .65f);
    if (elapsed < windup) return -1.0f + 2.0f * (elapsed - windup * .65f) / (windup * .35f);
    return 1.0f - std::clamp((elapsed - windup) / (Duration(style) - windup), 0.0f, 1.0f);
}
inline float Draw(float remaining)
{
    const float elapsed = Duration(2) - std::clamp(remaining, 0.0f, Duration(2));
    return elapsed < Windup(2) ? elapsed / Windup(2) : std::max(0.0f, 1.0f - (elapsed - Windup(2)) / .08f);
}
}
