#pragma once
#include "GameState.h"
#include "ChatacabraHDFrames.h"
#include "WorldLayout.h"
#include <algorithm>
#include <cmath>

namespace BossIntro
{
inline float Duration(const GameState &state)
{
    return state.bossDefeatIntro ? GameConstants::Chatacabra::DefeatDuration : GameConstants::Chatacabra::IntroDuration;
}
inline bool Active(const GameState &state)
{
    return state.bossIntroTime >= 0 && state.bossIntroTime < Duration(state) &&
        state.monster.definition.id == "ancient_guardian" &&
        (state.bossDefeatIntro ? state.monster.health <= 0 : state.monster.health > 0);
}
inline void Start(GameState &state)
{
    state.bossIntroPending = false;
    state.bossDefeatIntro = false;
    state.bossIntroTime = 0;
    state.inventoryOpen = false;
    state.moving = false;
    state.attack = state.dodge = state.hitStop = 0;
    state.playerHitTime = state.attackHitTime = 0;
    state.playerSlowTime = 0;
    state.damageNumbers.clear();
    if (state.monster.health > 0)
        state.monster.animationFacing = {state.player.x - state.monster.position.x, state.player.z - state.monster.position.z};
}
inline void Arm(GameState &state)
{
    state.bossIntroPending = true;
    state.bossDefeatIntro = false;
    state.bossIntroTime = -1;
}
inline bool TryStartVisible(GameState &state)
{
    if (!state.bossIntroPending || state.inventoryOpen || state.width <= 0 || state.height <= 0 ||
        state.monster.health <= 0 || state.monster.definition.id != "ancient_guardian") return false;
    const float dx = state.monster.position.x-state.player.x, dz = state.monster.position.z-state.player.z;
    const int direction = ChatacabraFrames::Direction(-dx,-dz);
    const auto &bounds = ChatacabraHDFrames::WalkBounds[direction];
    const float size = state.monster.definition.spriteHeight * GameConstants::Chatacabra::SpriteScale;
    const float scale = GameConstants::World::Scale * state.height / 720.0f * state.cameraZoom;
    const float x = (dx-dz)*32, y = (dx+dz)*16-86/state.cameraZoom +
        (WorldLayout::GroundHeight(state.monster.position)-WorldLayout::GroundHeight(state.player))*TerrainGeometry::HeightPixels;
    if ((x+bounds.right*size)*scale < -state.width*.5f || (x+bounds.left*size)*scale > state.width*.5f ||
        (y+bounds.top*size)*scale < -state.height*.5f || (y+bounds.bottom*size)*scale > state.height*.5f) return false;
    Start(state);
    return true;
}
inline void StartDefeat(GameState &state)
{
    Start(state);
    state.bossDefeatIntro = true;
    state.monster.deathTime = .01f;
    state.monster.hitFlash = 0;
    state.monster.chatImpactTime = 0;
}
inline void Skip(GameState &state)
{
    state.bossIntroPending = false;
    if (state.bossDefeatIntro)
        state.monster.deathTime = GameConstants::Chatacabra::DeathFrameDuration * 4;
    state.bossIntroTime = -1;
}
inline void Update(GameState &state, float deltaTime)
{
    if (!Active(state)) return;
    state.bossIntroTime += std::max(0.0f, deltaTime);
    if (state.bossDefeatIntro)
        state.monster.deathTime = GameConstants::Chatacabra::DeathFrameDuration * 3 *
            std::min(1.0f, state.bossIntroTime / GameConstants::Chatacabra::DefeatAnimationDuration);
    if (state.bossIntroTime >= Duration(state)) Skip(state);
}
inline float Smooth(float value)
{
    value = std::max(0.0f, std::min(1.0f, value));
    return value * value * (3 - 2 * value);
}
inline float Weight(const GameState &state)
{
    if (!Active(state)) return 0;
    using namespace GameConstants::Chatacabra;
    return std::min(Smooth(state.bossIntroTime / (state.bossDefeatIntro ? .8f : IntroPanDuration)),
        Smooth((Duration(state) - state.bossIntroTime) / (state.bossDefeatIntro ? 1.0f : IntroReturnDuration)));
}
inline float Sweep(const GameState &state)
{
    using namespace GameConstants::Chatacabra;
    return Smooth((state.bossIntroTime - IntroPanDuration) / (IntroDuration - IntroPanDuration - IntroReturnDuration));
}
inline float Zoom(const GameState &state)
{
    using namespace GameConstants::Chatacabra;
    const float target = state.bossDefeatIntro ? .58f : IntroZoom + (IntroZoomClose - IntroZoom) * Sweep(state);
    return state.cameraZoom + (target - state.cameraZoom) * Weight(state);
}
inline float FootOffset(const GameState &state)
{
    return 86 + 12 * Weight(state);
}
inline Vec2 Focus(const GameState &state)
{
    const float weight = Weight(state);
    Vec2 point = {state.player.x + (state.monster.position.x - state.player.x) * weight,
                  state.player.z + (state.monster.position.z - state.player.z) * weight};
    const float drift = state.bossDefeatIntro ? 0 : std::sin(Sweep(state) * 3.14159265f) * .55f * weight;
    point.x += drift;
    point.z -= drift;
    const float age = state.bossIntroTime - GameConstants::Chatacabra::IntroImpactTime;
    if (Active(state) && !state.bossDefeatIntro && age >= 0 && age < GameConstants::Chatacabra::IntroShakeDuration)
    {
        const float shake = GameConstants::Chatacabra::IntroShakeStrength *
            (1 - age / GameConstants::Chatacabra::IntroShakeDuration);
        point.x += std::sin(age * 75) * shake;
        point.z += std::cos(age * 61) * shake;
    }
    return point;
}
inline int SlamPose(float time)
{
    using namespace GameConstants::Chatacabra;
    return time < IntroPrepareTime || time >= IntroRecoverTime ? -1 : time < IntroRaiseTime ? 0 :
        time < IntroImpactTime ? 2 : 3;
}
inline float Fade(const GameState &state)
{
    return Active(state) ? std::min(Smooth(state.bossIntroTime / .5f),
        Smooth((Duration(state) - state.bossIntroTime) / .5f)) : 0;
}
}
