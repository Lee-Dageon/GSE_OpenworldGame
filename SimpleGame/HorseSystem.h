#pragma once
#include "GameState.h"
#include "Navigation.h"
#include "RegionSystem.h"
#include <utility>

namespace HorseSystem
{
inline bool Allowed(const GameState &state, Vec2 point)
{
    return !RegionSystem::Blocked(state, point) &&
        !(((point.x >= 790 && point.z >= 620) || point.x >= 940) && state.endingChoice == 0);
}
inline bool ClearApproach(const GameState &state, Vec2 from, const NavigationRoute &route)
{
    for (size_t i = route.next; i < route.points.size(); ++i)
    {
        const Vec2 to = route.points[i];
        const int steps = std::max(1, static_cast<int>(std::ceil(Navigation::Distance(from, to) / .12f)));
        for (int step = 1; step <= steps; ++step)
        {
            const float t = static_cast<float>(step) / steps;
            if (!Allowed(state, {from.x + (to.x-from.x)*t, from.z + (to.z-from.z)*t})) return false;
        }
        from = to;
    }
    return true;
}
inline void Dismount(GameState &state)
{
    if (!state.horse.mounted) return;
    state.horse.position = state.player;
    state.horse.facing = state.facing;
    state.horse.mounted = state.horse.called = state.horse.moving = false;
}
inline const wchar_t *Call(GameState &state)
{
    auto &horse = state.horse;
    if (state.inventoryOpen || state.journalOpen || state.health <= 0 ||
        state.dodge > 0 || state.attack > 0 || state.attackPending) return L"지금은 말을 부를 수 없습니다.";
    if (horse.mounted) { Dismount(state); return L"말에서 내렸습니다. H로 다시 부르세요."; }
    if (horse.called) { horse.called = horse.moving = false; return L"말 호출을 취소했습니다."; }
    NavigationRoute route;
    bool found = horse.visible && Navigation::Distance(horse.position, state.player) < GameConstants::Horse::RecallDistance &&
        Navigation::Plan(state.world, horse.position, state.player, route) && ClearApproach(state, horse.position, route);
    if (!found)
    {
        // Pick a reachable approach point; never spawn in water, a building or across a locked gate.
        for (float radius : {12.0f, 8.0f, 4.0f, 1.5f})
        {
            for (int i = 0; i < 16; ++i)
            {
                const float angle = i * .392699082f;
                const Vec2 point = {state.player.x + std::cos(angle)*radius, state.player.z + std::sin(angle)*radius};
                if (Allowed(state, point) && Navigation::Plan(state.world, point, state.player, route) &&
                    ClearApproach(state, point, route))
                { horse.position = point; found = true; break; }
            }
            if (found) break;
        }
    }
    if (!found) return L"말이 접근할 길이 없습니다. 넓은 곳에서 H를 누르세요.";
    horse.route = std::move(route);
    horse.visible = horse.called = true;
    horse.moving = false;
    horse.stalledTime = 0;
    return L"말을 불렀습니다! 가까워지면 자동 탑승 · H 하차";
}
inline bool Update(GameState &state, float deltaTime)
{
    auto &horse = state.horse;
    horse.moving = false;
    if (horse.mounted)
    { horse.position = state.player; horse.facing = state.facing; horse.moving = state.moving; return false; }
    if (!horse.called || state.health <= 0 || state.inventoryOpen || state.journalOpen || state.lootTime > 0) return false;
    const float dt = std::clamp(deltaTime, 0.0f, 1.0f);
    const Vec2 before = horse.position;
    if (Navigation::Distance(horse.route.target, state.player) > .4f || horse.route.next >= horse.route.points.size())
    {
        horse.route.retry = std::max(0.0f, horse.route.retry-dt);
        if (horse.route.retry <= 0)
        {
            Navigation::Plan(state.world, horse.position, state.player, horse.route);
            if (!ClearApproach(state, horse.position, horse.route)) horse.route.points.clear();
        }
    }
    // Follow only a checked route: Navigation::Move's direct fallback does not know regional gates.
    if (horse.route.next < horse.route.points.size())
        horse.moving = Navigation::Move(state.world, horse.position, horse.route.target,
            GameConstants::Horse::Speed*dt, dt, horse.route);
    if (horse.moving)
    {
        const float distance = Navigation::Distance(before, horse.position);
        if (distance > .001f) horse.facing = {(horse.position.x-before.x)/distance, (horse.position.z-before.z)/distance};
        horse.stalledTime = 0;
    }
    else horse.stalledTime += dt;
    if (horse.stalledTime > 5.0f || Navigation::Distance(horse.position, state.player) > GameConstants::Horse::RecallDistance)
    {
        horse.called = false;
        state.message = L"말이 멈췄습니다. 길이 열린 곳에서 H로 다시 부르세요.";
        state.messageTime = 5;
        return false;
    }
    if (Navigation::Distance(horse.position, state.player) > GameConstants::Horse::MountRange ||
        state.attack > 0 || state.attackPending || state.dodge > 0 ||
        !WorldLayout::CanTravel(state.world, horse.position, state.player) ||
        !Allowed(state, state.player)) return false;
    horse.mounted = true;
    horse.called = horse.moving = false;
    horse.position = state.player;
    horse.facing = state.facing;
    return true;
}
}
