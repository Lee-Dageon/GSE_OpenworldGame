#include "../SimpleGame/Monster.h"
#include "../SimpleGame/GameConstants.h"
#include "../SimpleGame/ChatacabraFrames.h"
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
    const Vec2 screenDirections[8]={{0,-1},{-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1}};
    for (int direction=0;direction<8;++direction)
    {
        const auto screen=screenDirections[direction];
        assert(ChatacabraFrames::Direction((screen.x+2*screen.z)*.5f,(2*screen.z-screen.x)*.5f)==direction);
    }
    assert(ChatacabraFrames::Direction(0,0)==0);
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

    monster=Create(); monster.health=1;
    const auto fatalHit=MonsterSystem::Hit(monster,{1,0},1);
    assert(fatalHit.defeated && monster.deathTime>0);
    assert(monster.animationFacing.x==1 && monster.animationFacing.z==0);
    MonsterSystem::UpdateTimers(monster,1);
    Tick(monster,{-1,0},1,health);
    assert(ChatacabraFrames::DeathPose(monster.deathTime)==3);
    assert(monster.animationFacing.x==1 && monster.animationFacing.z==0);
    monster=Create(); monster.health=1; monster.chatPattern=0; monster.animationFacing={0,1};
    MonsterSystem::Hit(monster,{1,0},1);
    assert(monster.animationFacing.x==0 && monster.animationFacing.z==1); // Keep committed attack direction.
    monster=Create(); monster.health=1; monster.chatPattern=0; monster.chatArmorHits=1; monster.animationFacing={0,1};
    MonsterSystem::Hit(monster,{1,0},1);
    assert(monster.health==0 && monster.animationFacing.x==0 && monster.animationFacing.z==1);

    monster=Create(); monster.definition.id="boar"; monster.definition.attackRange=2;
    health=100;
    assert(Tick(monster,{1,0},.01f,health) && health==78); // Other monsters keep contact attack.
}
