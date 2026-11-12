#pragma once
#include <array>

namespace Chapter1BoarAnimation
{
enum Action { Attack, Hit, Death };
constexpr std::array<std::array<int, 6>, 3> FrameMilliseconds = {{
    {{120, 160, 100, 120, 120, 160}},
    {{40, 60, 100, 60, 60, 80}},
    {{120, 140, 160, 160, 180, 240}}
}};
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
