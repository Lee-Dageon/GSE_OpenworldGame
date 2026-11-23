#include "stdafx.h"
#include "GameConstants.h"
#include "PlayerController.h"
#include "WorldLayout.h"
#include "RegionSystem.h"

#include <algorithm>
#include <cmath>

namespace PlayerController
{
namespace
{
Vec2 InputDirection(const GameState &state)
{
    Vec2 move={static_cast<float>(state.keys['w']-state.keys['s']-state.keys['a']+state.keys['d']),
               static_cast<float>(state.keys['w']-state.keys['s']+state.keys['a']-state.keys['d'])};
    const float length=std::hypot(move.x,move.z);
    if (length>0) { move.x/=length; move.z/=length; }
    return move;
}
}
void UpdateMovement(GameState &state, float deltaTime, float dodgeAtFrameStart)
{
    if (state.inventoryOpen || state.journalOpen || state.lootTime>0)
    { state.moving=false; return; }
    const float dt=std::clamp(deltaTime,0.0f,1.0f);
    const float rollTime=std::min(dt,std::max(0.0f,dodgeAtFrameStart<0 ? state.dodge : dodgeAtFrameStart));
    const Vec2 previous=state.player,input=InputDirection(state);
    const auto advance=[&](Vec2 move,float distance)
    {
        if (distance<=0 || std::hypot(move.x,move.z)<.001f) return;
        const int steps=std::max(1,static_cast<int>(std::ceil(distance/.12f)));
        for (int i=0;i<steps;++i)
        {
            const Vec2 next={state.player.x+move.x*distance/steps,state.player.z+move.z*distance/steps};
            const auto canMove=[&](Vec2 point) {
                return !((point.x>=790 && point.z>=620 || point.x>=940) && state.endingChoice==0) &&
                    !RegionSystem::Blocked(state,point) && WorldLayout::CanMove(state.world,state.player,point);
            };
            if (canMove(next)) state.player=next;
            else
            {
                const Vec2 x={next.x,state.player.z}; if (canMove(x)) state.player=x;
                const Vec2 z={state.player.x,next.z}; if (canMove(z)) state.player=z;
            }
        }
    };
    if (rollTime>0)
    { state.facing=state.dodgeDirection; advance(state.dodgeDirection,rollTime*GameConstants::Player::DodgeSpeed); }
    if (dt>rollTime && std::hypot(input.x,input.z)>.001f)
    {
        state.facing=input;
        const float slow=state.playerSlowTime>0 ? GameConstants::Player::HitSlowSpeed : 1.0f;
        const float weather=state.weather==3 ? .88f : state.weather==4 ? .93f : 1;
        advance(input,(dt-rollTime)*(state.sprint ? GameConstants::Player::SprintSpeed : GameConstants::Player::WalkSpeed)*slow*weather);
    }
    state.moving=state.player.x!=previous.x || state.player.z!=previous.z;
}

bool TryUsePotion(GameState &state)
{
    if (state.inventory.Count(ItemId::Potion) <= 0 || state.health >= state.maxHealth)
        return false;
    state.inventory.Remove(ItemId::Potion);
    state.health = std::min(state.maxHealth, state.health + GameConstants::Player::PotionHealing);
    return true;
}

bool StartDodge(GameState &state)
{
    if (state.dodge>0 || state.dodgeCooldown>0 || state.inventoryOpen || state.journalOpen ||
        state.lootTime>0 || state.health<=0) return false;
    Vec2 direction=InputDirection(state);
    if (std::hypot(direction.x,direction.z)<.001f) direction=state.facing;
    const float length=std::hypot(direction.x,direction.z);
    state.dodgeDirection=length>.001f ? Vec2{direction.x/length,direction.z/length} : Vec2{.70710678f,.70710678f};
    state.facing=state.dodgeDirection;
    state.dodge=GameConstants::Player::DodgeDuration;
    state.dodgeCooldown=GameConstants::Player::DodgeCooldown;
    state.attack=0; state.attackPending=false;
    return true;
}

} // namespace PlayerController
