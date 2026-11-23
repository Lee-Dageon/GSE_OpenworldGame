#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/Monster.h"
#include "../SimpleGame/GameContent.h"
#include "../SimpleGame/WeaponMotion.h"
#include "../SimpleGame/BossIntro.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    for (int style=0;style<3;++style)
    {
        bool pending=true;
        const float duration=WeaponMotion::Duration(style),windup=WeaponMotion::Windup(style);
        if (style) assert(!WeaponMotion::Release(style,duration-windup+.001f,pending));
        assert(WeaponMotion::Release(style,duration-windup,pending));
        assert(!WeaponMotion::Release(style,0,pending));
        assert(WeaponMotion::Progress(style,duration)==0 && WeaponMotion::Progress(style,0)==1);
    }
    assert(std::abs(WeaponMotion::Reach(1,WeaponMotion::Duration(1)-WeaponMotion::Windup(1))-1)<.001f);
    assert(WeaponMotion::Reach(1,0)==0);
    assert(WeaponMotion::Draw(.65f)==0 && WeaponMotion::Draw(0)==0);
    assert(std::abs(WeaponMotion::Draw(.4f)-1)<.001f);
    for (int style=1;style<=2;++style)
    {
        GameState s=CreateInitialGameState(); s.fieldMonsters.clear(); s.animals.clear();
        s.player={100,-100}; s.quest=QuestStage::Hunt;
        s.spearCrafted=s.bowCrafted=true; s.weaponStyle=style;
        s.monster=MonsterSystem::Create(GameContent::Get().monsters.at("forest_boar"),{102,-100});
        GameManager game(s); const int before=s.monster.health;
        game.HandleCommand(GameCommand::Attack);
        assert(s.attackPending && s.monster.health==before);
        game.HandleCommand(GameCommand::SwitchStyle);
        assert(s.weaponStyle==style);
        const float remaining=s.attack;
        s.inventoryOpen=true; game.UpdateGame(.5f); s.inventoryOpen=false;
        s.journalOpen=true; game.UpdateGame(.5f); s.journalOpen=false;
        s.hitStop=.5f; game.UpdateGame(.5f);
        assert(s.attack==remaining && s.attackPending && s.monster.health==before);
        game.UpdateGame(.1f);
        assert(s.attackPending && s.monster.health==before);
        game.HandleCommand(GameCommand::Attack); // Holding/clicking again cannot restart the windup.
        game.UpdateGame(style==1 ? .07f : .16f);
        assert(!s.attackPending && s.monster.health<before);
        const int after=s.monster.health;
        game.UpdateGame(1);
        assert(s.monster.health==after && !s.attackPending);
        game.HandleCommand(GameCommand::SwitchStyle);
        assert(s.weaponStyle!=style);
    }
    GameState miss=CreateInitialGameState(); miss.fieldMonsters.clear(); miss.animals.clear();
    miss.player={100,-100}; miss.weaponStyle=2; miss.bowCrafted=true;
    GameManager game(miss); game.HandleCommand(GameCommand::Attack); game.UpdateGame(.26f);
    assert(!miss.attackPending && miss.arrowTime>0); // Misses still release an arrow.
    miss.attackPending=true; miss.attack=.4f;
    BossIntro::Start(miss);
    assert(!miss.attackPending && miss.attack==0);
    std::cout << "PASS: weapon windup, single release, pause, style lock, recovery and missed arrow\n";
}
