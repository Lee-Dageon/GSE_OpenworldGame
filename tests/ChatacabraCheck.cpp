#include "../SimpleGame/Monster.h"
#include "../SimpleGame/GameConstants.h"
#include "../SimpleGame/ChatacabraFrames.h"
#include "../SimpleGame/ChatacabraHDFrames.h"
#include <cassert>

namespace
{
MonsterState Create()
{
    MonsterDefinition definition;
    definition.id="ancient_guardian";
    definition.health=240;
    definition.hitRange=2.1f;
    definition.chaseRange=9;
    definition.damage=28;
    return MonsterSystem::Create(definition,{0,0});
}
bool Tick(MonsterState &monster, Vec2 player, float dt, float &health, float dodge=0)
{
    return MonsterSystem::UpdateChaseAndAttack(monster,player,dodge,dt,health);
}
}

int main()
{
    const Vec2 screenDirections[8]={{0,-1},{-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1}};
    for (int direction=0;direction<8;++direction)
    {
        const auto screen=screenDirections[direction];
        assert(ChatacabraFrames::Direction((screen.x+2*screen.z)*.5f,(2*screen.z-screen.x)*.5f)==direction);
    }
    assert(ChatacabraFrames::Direction(0,0)==0);
    for (const auto &sheet : ChatacabraHDFrames::Frames)
        for (const auto &frame : sheet)
        {
            assert(frame.x > 0 && frame.x < 1 && frame.y > 0 && frame.y < 1);
            assert(frame.widthScale > .1f && frame.widthScale < 2);
            assert(frame.widthScale == frame.heightScale);
        }
    for (const auto &sheet : ChatacabraFrames::Frames)
        for (const auto &frame : sheet)
        {
            assert(frame.x>0 && frame.x<1 && frame.y>0 && frame.y<1);
            assert(frame.widthScale>.1f && frame.widthScale<2);
            assert(frame.widthScale==frame.heightScale); // Square cells, uniform scaling.
        }
    for (const auto &frame : ChatacabraFrames::DeathFrames)
    {
        assert(frame.x>0 && frame.x<1 && frame.y>0 && frame.y<1);
        assert(frame.widthScale>.1f && frame.widthScale<2);
        assert(frame.widthScale==frame.heightScale);
    }
    assert(ChatacabraFrames::DeathPose(-1)==0 && ChatacabraFrames::DeathPose(0)==0);
    for (int frame=0;frame<4;++frame)
        assert(ChatacabraFrames::DeathPose((frame+.5f)*GameConstants::Chatacabra::DeathFrameDuration)==frame);
    assert(ChatacabraFrames::DeathPose(100)==3); // Corpse holds the final pose instead of looping.
    using namespace GameConstants::Chatacabra;
    using namespace MonsterSystem;
    float health = 1000;
    auto monster = Create();
    Tick(monster, {1,0}, 0, health);
    assert(monster.chatPattern == Slam && monster.chatCombo);
    assert(!Tick(monster, {1,0}, FirstImpact-.01f, health));
    assert(Tick(monster, {1,0}, .02f, health));
    const float once = health;
    assert(!Tick(monster, {1,0}, .01f, health) && health == once);
    assert(Tick(monster, {1,0}, SecondImpact-FirstImpact, health));
    Tick(monster, {8,0}, SlamDuration, health);
    assert(monster.chatPattern == Tongue && monster.chatElapsed == 0 && monster.chatWeakTime == 0);
    Tick(monster, {8,0}, TongueDuration, health, 1);
    assert(monster.chatPattern == -1 && monster.chatWeakTime == WeakDuration);
    assert(monster.attackCooldown >= monster.chatWeakTime);
    const int beforeWeak = monster.health;
    Hit(monster, ChatWeakPoint(monster), 8);
    assert(monster.health == beforeWeak-16);
    const int beforeBody = monster.health;
    Hit(monster, {-1,0}, 8);
    assert(monster.health == beforeBody-8);
    UpdateTimers(monster, WeakDuration+.01f);
    const int closed = monster.health;
    Hit(monster, ChatWeakPoint(monster), 8);
    assert(monster.health == closed-8);

    monster = Create(); health = 1000;
    Tick(monster, {8,0}, 0, health);
    assert(monster.chatPattern == Tongue);
    assert(Tick(monster, {TongueRange-.01f,TongueHalfWidth-.01f}, FirstImpact+.01f, health));
    for (Vec2 outside : {Vec2{TongueRange+.01f,0},Vec2{8,TongueHalfWidth+.01f}})
    {
        monster=Create(); health=1000;
        Tick(monster,{8,0},0,health);
        assert(!Tick(monster,outside,FirstImpact+.01f,health) && health==1000);
    }
    monster=Create(); health=1000;
    Tick(monster,{8,0},0,health);
    assert(!Tick(monster,{8,0},FirstImpact+.01f,health,1));
    assert(!Tick(monster,{8,0},.1f,health) && health==1000);
    monster=Create(); health=1000;
    Tick(monster,{1,0},0,health);
    assert(!Tick(monster,{1+SlamRadius+.01f,0},SecondImpact+.01f,health));
    monster=Create(); monster.chatArmorHits=ArmorBreakHits;
    Tick(monster,{1,0},0,health);
    assert(Tick(monster,{1+SlamRadius+.1f,0},FirstImpact+.01f,health));

    monster=Create(); monster.chatAttacks=2; health=1000;
    Tick(monster,{1,0},0,health);
    assert(monster.chatPattern==Coat);
    assert(!Tick(monster,{1,0},CoatTime+.01f,health));
    assert(monster.chatArmorHits==ArmorBreakHits);
    const int armored=monster.health;
    Hit(monster,{-1,0},10);
    assert(monster.chatArmorHits==ArmorBreakHits && monster.health==armored-4);
    for (int index=0;index<ArmorBreakHits;++index) Hit(monster,ChatArmPoint(monster,1),1);
    assert(monster.chatArmorHits==0 && monster.chatStagger==StaggerDuration && !monster.chatCombo);
    assert(monster.chatArmorBreakTime==ArmorBreakEffectDuration && monster.chatArmorCooldown>0);
    assert(!Tick(monster,{1,0},.2f,health));
    UpdateTimers(monster,StaggerDuration+.01f);
    assert(monster.chatStagger==0);
    Tick(monster,{1,0},0,health);
    assert(monster.chatPattern==Slam); // Cannot immediately refresh broken armor.

    monster=Create(); health=1000;
    monster.health=monster.maxHealth/2;
    Tick(monster,{8,0},0,health);
    assert(monster.chatRage && monster.chatPattern==Roar);
    assert(!Tick(monster,{8,0},RoarImpactTime+.01f,health) && monster.chatRageTime>0);
    Tick(monster,{8,0},RoarDuration,health);
    UpdateTimers(monster,1);
    Tick(monster,{8,0},0,health);
    assert(monster.chatPattern==Tongue);
    assert(Tick(monster,{8,0},(FirstImpact+.01f)/RageSpeed,health));
    assert(health<1000-28);

    monster=Create(); health=1000;
    Tick(monster,{15,0},0,health);
    assert(monster.chatPattern==Charge && monster.chatTarget.x==15);
    assert(!Tick(monster,{8,0},ChargeWindup-.01f,health));
    assert(Tick(monster,{8,0},ChargeTravelTime+.02f,health)); // Swept hit, even in a long frame.
    assert(monster.position.x==15);
    const float charged=health;
    assert(!Tick(monster,{15,0},.1f,health) && health==charged);
    monster=Create(); health=1000;
    Tick(monster,{15,0},0,health);
    assert(!Tick(monster,{8,ChargeHalfWidth+.01f},ChargeWindup+ChargeTravelTime,health));
    monster=Create();
    Tick(monster,{15,0},0,health);
    assert(!Tick(monster,{8,0},ChargeWindup+ChargeTravelTime,health,1));
    assert(!Tick(monster,{15,0},.01f,health));

    monster=Create(); monster.health=1; monster.chatPattern=Tongue; monster.animationFacing={0,1};
    assert(Hit(monster,{1,0},1).defeated);
    assert(monster.animationFacing.x==0 && monster.animationFacing.z==1);
    assert(!Tick(monster,{0,1},1,health));
    UpdateTimers(monster,1);
    assert(ChatacabraFrames::DeathPose(monster.deathTime)==3);
    MonsterDefinition ordinary;
    ordinary.id="boar"; ordinary.health=10; ordinary.hitRange=2.1f;
    ordinary.chaseRange=9; ordinary.attackRange=2; ordinary.damage=22;
    monster=MonsterSystem::Create(ordinary,{0,0}); health=100;
    assert(monster.definition.hitRange==2.1f && monster.definition.chaseRange==9);
    assert(Tick(monster,{1,0},.01f,health) && health==78);
}
