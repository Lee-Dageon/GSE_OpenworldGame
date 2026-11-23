#include "../SimpleGame/LivingWorld.h"
#include <cassert>
#include <iostream>

int main()
{
    auto s=CreateInitialGameState();
    const Vec2 left={-9,-1.7f},right={-3,-1.7f};
    assert(!WorldLayout::IsBlocked(left) && !WorldLayout::IsBlocked(right));
    assert(!WorldLayout::ClearLine(left,right));
    auto definition=GameContent::Get().monsters.at("forest_boar");
    definition.attackRange=10; definition.chaseRange=20; definition.speed=6;
    auto monster=MonsterSystem::Create(definition,left);
    float health=100;
    assert(!MonsterSystem::UpdateWorldCombat(monster,s.world,right,0,0,health));
    assert(health==100);
    assert(!WorldLayout::CanTravel(s.world,left,right,.12f));
    assert(WorldLayout::CanTravel(s.world,{100,-100},{106,-100},.12f));
    monster=MonsterSystem::Create(definition,{100,-100});
    assert(MonsterSystem::UpdateWorldCombat(monster,s.world,{101,-100},0,0,health,3));
    assert(health==100-std::max(1.0f,definition.damage-3));
    monster.attackCooldown=0; const float damaged=health;
    assert(!MonsterSystem::UpdateWorldCombat(monster,s.world,{101,-100},1,0,health));
    assert(health==damaged);

    // Both chase and committed leaps must test the path, not only the clear endpoint.
    definition.attackRange=.5f;
    monster=MonsterSystem::Create(definition,left);
    MonsterSystem::UpdateWorldCombat(monster,s.world,right,0,1,health);
    assert(monster.position.x==left.x && monster.position.z==left.z);
    for (const auto &id : {"forest_predator","ancient_guardian"})
    {
        auto leap=MonsterSystem::Create(GameContent::Get().monsters.at(id),left);
        leap.chatPattern=id==std::string("forest_predator") ? 0 : MonsterSystem::Charge;
        leap.chatOrigin=left; leap.chatTarget=right; leap.animationFacing={1,0};
        leap.chatElapsed=id==std::string("forest_predator") ? .7f : GameConstants::Chatacabra::ChargeWindup;
        const float dt=id==std::string("forest_predator") ? .5f : GameConstants::Chatacabra::ChargeTravelTime;
        health=100;
        assert(!MonsterSystem::UpdateWorldCombat(leap,s.world,right,0,dt,health));
        assert(leap.position.x==left.x && health==100);
    }
    auto guardian=MonsterSystem::Create(GameContent::Get().monsters.at("mutated_guardian"),left);
    guardian.chatPattern=0; guardian.chatElapsed=1; guardian.chatTarget=right;
    assert(!MonsterSystem::UpdateWorldCombat(guardian,s.world,right,0,.2f,health));
    assert(health==100);

    // The same tree separates the companion/NPC from a nearby field enemy.
    const Vec2 tree=WorldLayout::Trees()[0].position;
    const Vec2 a={tree.x-.7f,tree.z},b={tree.x+.7f,tree.z};
    assert(!WorldLayout::ClearLine(a,b));
    s.fieldMonsters.clear(); s.animals.clear(); s.npcs.clear();
    s.player=s.companion=a; s.baseLevels[3]=1;
    definition.speed=0; definition.hitRange=3;
    FieldMonster field; field.home=b; field.monster=MonsterSystem::Create(definition,b);
    field.monster.attackCooldown=10; s.fieldMonsters.push_back(field);
    WorldStreaming::Update(s);
    const int enemyHealth=field.monster.health;
    LivingWorld::Update(s,0);
    assert(s.fieldMonsters[0].monster.health==enemyHealth && s.companionCooldown==0);
    s.companion=s.player={100,-100}; s.fieldMonsters[0].home={101,-100};
    s.fieldMonsters[0].monster.position={101,-100}; WorldStreaming::Update(s);
    LivingWorld::Update(s,0);
    assert(s.fieldMonsters[0].monster.health<enemyHealth && s.companionCooldown==1);
    s.baseLevels[3]=0; s.player={a.x-10,a.z};
    s.fieldMonsters[0]=field; s.fieldMonsters[0].monster.attackCooldown=0;
    NpcState npc; npc.position=a; npc.health=100; s.npcs.push_back(npc);
    WorldStreaming::Update(s); LivingWorld::Update(s,0);
    assert(s.npcs[0].health==100);
    s.npcs[0].position={b.x+1,b.z};
    LivingWorld::Update(s,0);
    assert(s.npcs[0].health<100); // Open nearby residents remain vulnerable.
    s.npcs.clear(); s.fieldMonsters[0]=field; s.fieldMonsters[0].monster.attackCooldown=0;
    FieldMonster rival=field; rival.home=a; rival.monster.position=a;
    rival.monster.definition.id="forest_predator"; rival.monster.attackCooldown=10;
    s.fieldMonsters.push_back(rival);
    LivingWorld::Update(s,0);
    assert(s.fieldMonsters[1].monster.health==enemyHealth); // Territory fights also respect cover.
    Vec2 follower=left; LivingWorld::Approach(s.world,follower,right,6);
    assert(follower.x==left.x && follower.z==left.z);
    std::cout << "PASS: blocked melee/area hits, swept chase/leap/charge, dodge/defense, companion/NPC barriers and follow collision\n";
}
