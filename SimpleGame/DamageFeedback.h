#pragma once
#include <algorithm>
#include "GameConstants.h"

namespace DamageFeedback
{
using namespace GameConstants::Feedback;
inline float Elapsed(float remaining) { return Duration - remaining; }
inline float Fade(float remaining, float duration)
{
    return remaining > 0 ? std::max(0.0f, std::min(1.0f, 1.0f - Elapsed(remaining) / duration)) : 0.0f;
}
} // namespace DamageFeedback
