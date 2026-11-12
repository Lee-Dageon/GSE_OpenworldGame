#include "stdafx.h"
#include "GameConstants.h"
#include "PlayerController.h"
#include "WorldLayout.h"

#include <algorithm>
#include <cmath>

namespace PlayerController
{
void UpdateMovement(GameState &state, float deltaTime)
{
    if (state.inventoryOpen || state.lootTime > 0)
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
    const float speed = state.dodge > 0.0f ? GameConstants::Player::DodgeSpeed : (state.sprint ? GameConstants::Player::SprintSpeed : GameConstants::Player::WalkSpeed);
    Vec2 next = {state.player.x + move.x * speed * deltaTime, state.player.z + move.z * speed * deltaTime};
    next.x = std::max(state.world.minimumCoordinate, std::min(state.world.maximumCoordinate, next.x));
    next.z = std::max(state.world.minimumCoordinate, std::min(state.world.maximumCoordinate, next.z));
    const auto canMove = [&](Vec2 position) {
        const float dx = position.x - state.world.lake.x, dz = position.z - state.world.lake.z;
        return dx*dx + dz*dz > state.world.lakeRadius*state.world.lakeRadius && !WorldLayout::IsBlocked(position);
    };
    const Vec2 previous = state.player;
    if (canMove(next)) state.player = next;
    else
    {
        const Vec2 alongX = {next.x, previous.z};
        if (canMove(alongX)) state.player = alongX;
        const Vec2 alongZ = {state.player.x, next.z};
        if (canMove(alongZ)) state.player = alongZ;
    }
    state.moving = state.player.x != previous.x || state.player.z != previous.z;
}

bool TryUsePotion(GameState &state)
{
    if (state.inventory.Count(ItemId::Potion) <= 0 || state.health >= state.maxHealth)
        return false;
    state.inventory.Remove(ItemId::Potion);
    state.health = std::min(state.maxHealth, state.health + GameConstants::Player::PotionHealing);
    return true;
}

void StartDodge(GameState &state)
{
    state.dodge = GameConstants::Player::DodgeDuration;
}
} // namespace PlayerController
