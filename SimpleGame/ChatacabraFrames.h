#pragma once
#include "GameConstants.h"

namespace ChatacabraFrames
{
struct Pivot { float x, y; };
// Alpha >= 128 foot bounds; tongue tip is excluded from the front extension anchor.
constexpr Pivot Pivots[3][16] = {
    {{.5f,.7827476f},{.5f,.7827476f},{.5f,.7827476f},{.5f,.7859425f},
     {.5f,.7175159f},{.5f,.7452229f},{.5f,.7452229f},{.5f,.7452229f},
     {.5f,.6773163f},{.5f,.6996805f},{.5f,.6964856f},{.5f,.7028754f},
     {.5f,.6146497f},{.5f,.6242038f},{.5f,.6305732f},{.5f,.6305732f}},
    {{.5f,.8881789f},{.5f,.8849840f},{.5f,.8881789f},{.5f,.8785942f},
     {.5f,.8853503f},{.5f,.8757962f},{.5f,.8789809f},{.5f,.8917197f},
     {.5f,.8626198f},{.5f,.8753994f},{.5f,.8849840f},{.5f,.8785942f},
     {.5f,.7993631f},{.5f,.7993631f},{.5f,.7993631f},{.5f,.8152866f}},
    {{.5f,.8338658f},{.5f,.8338658f},{.5f,.8338658f},{.5f,.8338658f},
     {.5f,.7993631f},{.5f,.8089172f},{.5f,.8057325f},{.5f,.8121019f},
     {.5f,.7795527f},{.5f,.7827476f},{.5f,.7827476f},{.5f,.7987220f},
     {.5f,.7452229f},{.5f,.7484076f},{.5f,.7515924f},{.5f,.7579618f}}
};
inline int Pose(int pattern, float time)
{
    using namespace GameConstants::Chatacabra;
    if (pattern==0) return time<FirstImpact ? 0 : time<TongueSweepTime ? 1 : time<TongueRetractTime ? 2 : 3;
    if (pattern==1) return time<FirstImpact ? 0 : time<SlamRaiseTime ? 1 : time<SecondImpact ? 2 : 3;
    return time<CoatLickTime ? 0 : time<CoatTime ? 1 : time<CoatReadyTime ? 2 : 3;
}
}
