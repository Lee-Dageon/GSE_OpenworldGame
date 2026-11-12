#pragma once
#include <algorithm>

namespace DamageFeedback
{
constexpr float HitStop = .06f;
constexpr float EffectDuration = .9f;
constexpr float Duration = HitStop + EffectDuration;
constexpr float FlashDuration = .16f;
constexpr float ShakeDuration = .25f;
constexpr float BorderDuration = .45f;
inline float Elapsed(float remaining) { return Duration - remaining; }
inline float Fade(float remaining, float duration)
{
    return remaining > 0 ? std::max(0.0f, std::min(1.0f, 1.0f - Elapsed(remaining) / duration)) : 0.0f;
}
} // namespace DamageFeedback
