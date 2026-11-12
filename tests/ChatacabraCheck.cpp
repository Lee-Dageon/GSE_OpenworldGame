#include "../SimpleGame/Monster.h"
#include "../SimpleGame/GameConstants.h"
#include <cassert>

namespace
{
MonsterState Create()
{
    MonsterDefinition definition;
    definition.id="ancient_guardian";
    definition.health=90;
    definition.hitRange=2.1f;
    definition.chaseRange=9;
    definition.damage=22;
    return MonsterSystem::Create(definition,{0,0});
}
bool Tick(MonsterState &monster, Vec2 player, float dt, float &health, float dodge=0)
{
    return MonsterSystem::UpdateChaseAndAttack(monster,player,dodge,dt,health);
}
}

int main()
{
    using namespace GameConstants::Chatacabra;
    auto monster=Create();
    float health=100;
    assert(!Tick(monster,{1,0},0,health)); // Start telegraph, no instant damage.
    assert(!Tick(monster,{1,0},FirstImpact-.01f,health));
    assert(Tick(monster,{1,0},.02f,health) && health==78);
    assert(!Tick(monster,{1,0},.1f,health) && health==78); // One hit per event.

    monster=Create(); health=100;
    Tick(monster,{1,0},0,health);
    assert(!Tick(monster,{1,0},FirstImpact+.01f,health,1));
    assert(!Tick(monster,{1,0},.1f,health) && health==100); // Dodged event cannot hit later.
    monster=Create();
    Tick(monster,{1,0},0,health);
    assert(!Tick(monster,{1,2},FirstImpact+.01f,health) && health==100); // Sidestep tongue.

    monster=Create(); monster.chatNext=1; health=100;
    Tick(monster,{1,0},0,health);
    assert(Tick(monster,{1,0},FirstImpact+.01f,health) && health==78);
    assert(Tick(monster,{1,0},SecondImpact-FirstImpact,health) && health==56);
    assert(!Tick(monster,{1,0},.1f,health) && health==56);
    MonsterSystem::Hit(monster,{1,0},7);
    assert(monster.health==90-7*WeakDamageMultiplier);
    Tick(monster,{1,0},SlamDuration,health);
    const int before=monster.health;
    MonsterSystem::Hit(monster,{1,0},7);
    assert(monster.health==before-7); // Weak window closes.
    monster=Create(); monster.chatNext=1; health=100;
    Tick(monster,{1,0},0,health);
    assert(!Tick(monster,{6,0},SecondImpact+.01f,health) && health==100); // Locked slam target.

    monster=Create(); monster.chatNext=2;
    Tick(monster,{1,0},0,health);
    assert(!Tick(monster,{1,0},CoatTime+.01f,health));
    assert(monster.chatArmorHits==ArmorBreakHits && health==100);
    for (int hit=0;hit<ArmorBreakHits;++hit) MonsterSystem::Hit(monster,{1,0},1);
    assert(monster.chatArmorHits==0 && monster.chatStagger==StaggerDuration && monster.chatPattern==-1);
    assert(!Tick(monster,{1,0},.1f,health));
    MonsterSystem::UpdateTimers(monster,StaggerDuration+.01f);
    assert(monster.chatStagger==0);

    monster=Create(); monster.chatNext=1; monster.chatArmorHits=ArmorBreakHits;
    health=100;
    Tick(monster,{1,0},0,health);
    assert(Tick(monster,{1+SlamRadius+.1f,0},FirstImpact+.01f,health));
    assert(health==100-22*ArmoredDamageMultiplier); // Enlarged radius and damage.
    monster.health=0;
    assert(!Tick(monster,{1,0},SecondImpact,health));

    monster=Create(); monster.definition.id="boar"; monster.definition.attackRange=2;
    health=100;
    assert(Tick(monster,{1,0},.01f,health) && health==78); // Other monsters keep contact attack.
}
