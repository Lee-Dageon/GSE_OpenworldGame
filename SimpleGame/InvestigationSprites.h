#pragma once
#include <string>

namespace InvestigationSprites
{
struct Asset { const wchar_t *file; float size, pivotX, pivotY; };
constexpr Asset Assets[] = {
    {L"tracks-v1.png", 110.0f, 0.50000000f, 0.47926635f},
    {L"camp-v1.png", 165.0f, 0.50000000f, 0.76035088f},
    {L"records-v1.png", 120.0f, 0.49960128f, 0.76347687f},
    {L"signal-v1.png", 145.0f, 0.49401914f, 0.85574163f},
    {L"fragment-v1.png", 80.0f, 0.49960128f, 0.57430622f},
    {L"entrance-v1.png", 205.0f, 0.50000000f, 0.88826156f},
    {L"control-v1.png", 135.0f, 0.50039872f, 0.82159490f},
};
struct Site { const char *stepId; int asset; };
constexpr Site Sites[] = {
    {"south_travel", 0},
    {"south_tracks", 0},
    {"forest_travel", 0},
    {"forest_tracks", 1},
    {"sand_travel", 5},
    {"sand_records", 2},
    {"ruin_signal", 3},
    {"ancient_fragment", 4},
    {"expedition_travel", 1},
    {"expedition_records", 2},
    {"ruin_entrance", 5},
    {"ruin_control", 6},
};
inline int Find(const std::string &stepId)
{
    for (const auto &site : Sites)
        if (stepId == site.stepId) return site.asset;
    return -1;
}
}
