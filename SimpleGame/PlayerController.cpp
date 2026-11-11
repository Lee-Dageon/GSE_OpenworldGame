#include "stdafx.h"
#include "PlayerController.h"
#include "WorldLayout.h"

#include <algorithm>
#include <cmath>

namespace PlayerController
{
void UpdateMovement(GameState &state, float deltaTime)
{
    if (state.inventoryOpen)
    {
        state.moving = false;
        return;
    }
    Vec2 move = {0.0f, 0.0f};
    if (state.keys['w'])
    {
        ++move.x;
        ++move.z;
    }
    if (state.keys['s'])
    {
        --move.x;
        --move.z;
    }
    if (state.keys['a'])
    {
        --move.x;
        ++move.z;
    }
    if (state.keys['d'])
    {
        ++move.x;
        --move.z;
    }
    const float length = std::sqrt(move.x * move.x + move.z * move.z);
    state.moving = length > 0.0f;
    if (!state.moving)
        return;
    move.x /= length;
    move.z /= length;
    state.facing = move;
    const float speed = state.dodge > 0.0f ? 9.0f : (state.sprint ? 4.0f : 2.4f);
    Vec2 next = {state.player.x + move.x * speed * deltaTime, state.player.z + move.z * speed * deltaTime};
    next.x = std::max(state.world.minimumCoordinate, std::min(state.world.maximumCoordinate, next.x));
    next.z = std::max(state.world.minimumCoordinate, std::min(state.world.maximumCoordinate, next.z));
    const float dx = next.x - state.world.lake.x;
    const float dz = next.z - state.world.lake.z;
    if (std::sqrt(dx * dx + dz * dz) > state.world.lakeRadius && !WorldLayout::IsBlocked(next))
        state.player = next;
}

bool TryUsePotion(GameState &state)
{
    if (state.inventory.Count(ItemId::Potion) <= 0 || state.health >= state.maxHealth)
        return false;
    state.inventory.Remove(ItemId::Potion);
    state.health = std::min(state.maxHealth, state.health + 45.0f);
    return true;
}

void StartDodge(GameState &state)
{
    state.dodge = .24f;
}
} // namespace PlayerController
