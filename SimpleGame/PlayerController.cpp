#include "stdafx.h"
#include "GameConstants.h"
#include "PlayerController.h"
#include "WorldLayout.h"
#include "RegionSystem.h"

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
    const float slow = state.playerSlowTime > 0 ? GameConstants::Player::HitSlowSpeed : 1.0f;
    const float weatherSpeed=state.weather==3 ? .88f : state.weather==4 ? .93f : 1;
    const float speed = state.dodge > 0.0f ? GameConstants::Player::DodgeSpeed :
        (state.sprint ? GameConstants::Player::SprintSpeed : GameConstants::Player::WalkSpeed) * slow * weatherSpeed;
    const Vec2 previous = state.player;
    const float distance=std::max(0.0f,std::min(deltaTime,1.0f))*speed;
    const int steps=std::max(1,static_cast<int>(std::ceil(distance/.12f)));
    for (int i=0;i<steps;++i)
    {
        const Vec2 next={state.player.x+move.x*distance/steps,state.player.z+move.z*distance/steps};
        const auto canMove=[&](Vec2 p) {
            return !((p.x>=790 && p.z>=620 || p.x>=940) && state.endingChoice==0) &&
                !RegionSystem::Blocked(state,p) && WorldLayout::CanMove(state.world,state.player,p);
        };
        if (canMove(next)) state.player=next;
        else
        {
            const Vec2 x={next.x,state.player.z};
            if (canMove(x)) state.player=x;
            const Vec2 z={state.player.x,next.z};
            if (canMove(z)) state.player=z;
        }
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
