#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/PlayerController.h"
#include "../SimpleGame/RollAnimation.h"
#include "../SimpleGame/SaveGame.h"
#include <cassert>
#include <iostream>

int main()
{
    const Vec2 directions[8]={{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0}};
    for (int i=0;i<8;++i)
    {
        auto s=CreateInitialGameState(); s.facing=directions[i];
        assert(PlayerController::StartDodge(s));
        assert(RollAnimation::Direction(s.dodgeDirection)==i);
        assert(std::abs(std::hypot(s.dodgeDirection.x,s.dodgeDirection.z)-1)<.0001f);
        for (int frame=0;frame<6;++frame)
        {
            const float remaining=GameConstants::Player::DodgeDuration*(1-(frame+.25f)/6);
            assert(RollAnimation::Frame(remaining)==frame);
            const auto region=RollAnimation::Frames[i][frame];
            assert(region.x>=0 && region.y>=0 && region.width>0 && region.height>0);
            assert(region.x+region.width<=RollAnimation::Width && region.y+region.height<=RollAnimation::Height);
        }
    }
    assert(RollAnimation::Frame(1)==0 && RollAnimation::Frame(-1)==5);
    auto s=CreateInitialGameState(); s.fieldMonsters.clear(); s.animals.clear(); s.npcs.clear();
    s.player={100,-100}; s.keys['s']=true; s.facing={1,0}; s.attack=.4f; s.attackPending=true;
    assert(PlayerController::StartDodge(s));
    assert(s.dodgeDirection.x<0 && s.dodgeDirection.z<0 && s.attack==0 && !s.attackPending);
    const Vec2 locked=s.dodgeDirection;
    s.keys={}; s.keys['w']=true;
    PlayerController::UpdateMovement(s,.1f);
    assert(s.player.x<100 && s.player.z<-100 && s.facing.x==locked.x);
    assert(!PlayerController::StartDodge(s));

    s=CreateInitialGameState(); s.fieldMonsters.clear(); s.animals.clear(); s.npcs.clear();
    s.player={100,-100}; s.facing={1,0}; GameManager game(s);
    assert(GameInput::OnKeyDown(s,' ',false)==GameCommand::Dodge);
    game.HandleCommand(GameCommand::Dodge);
    assert(GameInput::OnKeyDown(s,' ',false)==GameCommand::None);
    s.inventoryOpen=true; game.UpdateGame(.5f); s.inventoryOpen=false;
    s.journalOpen=true; game.UpdateGame(.5f); s.journalOpen=false;
    s.hitStop=.2f; game.UpdateGame(.2f);
    assert(s.dodgeCooldown==GameConstants::Player::DodgeCooldown && s.player.x==100);
    game.UpdateGame(.8f); // Only .36 seconds of a long frame may use roll speed.
    assert(std::abs(s.player.x-100-GameConstants::Player::DodgeSpeed*GameConstants::Player::DodgeDuration)<.002f);
    assert(s.dodge==0 && s.dodgeCooldown>0 && !PlayerController::StartDodge(s));
    game.UpdateGame(.31f);
    assert(s.dodgeCooldown==0 && GameInput::OnKeyDown(s,' ',false)==GameCommand::None);
    GameInput::OnKeyUp(s,' ');
    game.HandleCommand(GameInput::OnKeyDown(s,' ',false));
    assert(s.dodge>0 && s.dodgeCooldown==GameConstants::Player::DodgeCooldown);
    const float active=s.dodge; game.HandleCommand(GameCommand::Attack);
    assert(s.attack==0 && s.dodge==active);
    auto enemy=MonsterSystem::Create(GameContent::Get().monsters.at("forest_boar"),s.player);
    float health=100;
    assert(!MonsterSystem::UpdateChaseAndAttack(enemy,s.player,s.dodge,0,health) && health==100);
    game.UpdateGame(.4f);
    enemy.position=s.player;
    assert(s.dodge==0 && s.dodgeCooldown>0);
    assert(MonsterSystem::UpdateChaseAndAttack(enemy,s.player,s.dodge,0,health) && health<100);
    auto persisted=CreateInitialGameState();
    assert(PlayerController::StartDodge(persisted));
    GameState loaded;
    assert(SaveGame::Decode(SaveGame::Encode(persisted),persisted,loaded));
    assert(loaded.dodge==0 && loaded.dodgeCooldown==0);

    auto wall=CreateInitialGameState(); wall.player={-9,-1.7f}; wall.facing={1,0};
    assert(PlayerController::StartDodge(wall));
    PlayerController::UpdateMovement(wall,GameConstants::Player::DodgeDuration);
    assert(wall.player.x<-7.5f && WorldLayout::CanTravel(wall.world,{-9,-1.7f},wall.player));
    std::cout << "PASS: 8-direction roll frames, locked/input-facing motion, cooldown/held key, pause, long frame, collision and transient save\n";
}
