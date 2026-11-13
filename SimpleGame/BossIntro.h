#pragma once
#include "GameState.h"
#include <algorithm>
#include <cmath>

namespace BossIntro
{
inline bool Active(const GameState &state)
{
    return state.bossIntroTime >= 0 && state.bossIntroTime < GameConstants::Chatacabra::IntroDuration &&
        state.monster.definition.id == "ancient_guardian" && state.monster.health > 0;
}
inline void Start(GameState &state)
{
    state.bossIntroTime = 0;
    state.inventoryOpen = false;
    state.moving = false;
    state.attack = state.dodge = state.hitStop = 0;
    state.playerHitTime = state.attackHitTime = 0;
    state.damageNumbers.clear();
    state.monster.animationFacing = {state.player.x - state.monster.position.x, state.player.z - state.monster.position.z};
}
inline void Skip(GameState &state) { state.bossIntroTime = -1; }
inline float Smooth(float value)
{
    value = std::max(0.0f, std::min(1.0f, value));
    return value * value * (3 - 2 * value);
}
inline float Weight(const GameState &state)
{
    if (!Active(state)) return 0;
    using namespace GameConstants::Chatacabra;
    return std::min(Smooth(state.bossIntroTime / IntroPanDuration),
        Smooth((IntroDuration - state.bossIntroTime) / IntroReturnDuration));
}
inline float Zoom(const GameState &state)
{
    return state.cameraZoom + (GameConstants::Chatacabra::IntroZoom - state.cameraZoom) * Weight(state);
}
inline Vec2 Focus(const GameState &state)
{
    const float weight = Weight(state);
    Vec2 point = {state.player.x + (state.monster.position.x - state.player.x) * weight,
                  state.player.z + (state.monster.position.z - state.player.z) * weight};
    const float age = state.bossIntroTime - GameConstants::Chatacabra::IntroImpactTime;
    if (Active(state) && age >= 0 && age < .35f)
    {
        const float shake = .09f * (1 - age / .35f);
        point.x += std::sin(age * 75) * shake;
        point.z += std::cos(age * 61) * shake;
    }
    return point;
}
inline int SlamPose(float time)
{
    return time < .9f || time >= 2.25f ? -1 : time < 1.5f ? 0 :
        time < GameConstants::Chatacabra::IntroImpactTime ? 2 : 3;
}
inline float Fade(const GameState &state)
{
    return Active(state) ? std::min(Smooth(state.bossIntroTime / .5f),
        Smooth((GameConstants::Chatacabra::IntroDuration - state.bossIntroTime) / .5f)) : 0;
}
}
