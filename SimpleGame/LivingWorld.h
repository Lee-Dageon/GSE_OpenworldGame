#pragma once
#include "GameContent.h"
#include "WorldLayout.h"
#include "Navigation.h"
#include "WorldStreaming.h"
#include "InventoryActions.h"
#include "LootTable.h"
#include "QuestSystem.h"
#include "StorySystem.h"
#include <algorithm>
#include <cmath>

namespace LivingWorld
{
inline int Day(const GameState &s) { return static_cast<int>(s.worldTime/600); }
inline int Season(const GameState &s) { return Day(s)/7%4; }
inline const wchar_t *SeasonName(const GameState &s)
{
    const wchar_t *names[]={L"봄",L"여름",L"가을",L"겨울"}; return names[Season(s)];
}
inline const wchar_t *WeatherName(const GameState &s)
{
    const wchar_t *names[]={L"맑음",L"비",L"폭풍",L"가뭄",L"눈"}; return names[std::clamp(s.weather,0,4)];
}
inline const wchar_t *ContractName(int i)
{
    const wchar_t *names[]={L"주변 위협 토벌",L"자연 재료 채집",L"새 지역 정찰",L"주민 구조 지원",L"마을 방어"};
    return names[std::clamp(i,0,4)];
}
constexpr int Goals[]={2,6,3,1,2};
inline void Record(GameState &s,int kind,int amount=1)
{
    if (kind<0 || kind>=5 || amount<=0) return;
    auto &c=s.contracts[kind];
    if (c.active) c.progress=std::min(Goals[kind],c.progress+amount);
}
inline void Accept(GameState &s)
{
    if (!s.contracts[2].active) s.regionVisits=0;
    for (auto &c:s.contracts) if (!c.active) { c.active=true; c.progress=0; }
    if (s.rescuedSurvivors>0 && s.contracts[3].completed==0) Record(s,3,s.rescuedSurvivors);
}
inline const wchar_t *Report(GameState &s)
{
    if (!InventoryActions::AtStorage(s)) return L"장로 옆에서 완료 의뢰를 보고하세요.";
    int completed=0;
    for (int i=0;i<5;++i)
    {
        auto &c=s.contracts[i];
        if (c.active && c.progress>=Goals[i])
        { c.active=false; c.progress=0; ++c.completed; s.coins+=20+i*5; ++completed; }
    }
    if (completed) { QuestSystem::AddExperience(s,completed*15); return L"완료 의뢰를 보고했습니다. 주화와 경험치를 받았습니다."; }
    return L"완료한 의뢰가 없습니다. J에서 여러 의뢰를 함께 확인하세요.";
}
inline const wchar_t *Build(GameState &s)
{
    if (!InventoryActions::AtStorage(s)) return L"거점은 장로 옆에서 G로 확장합니다.";
    const int kind=s.nextBaseUpgrade, level=s.baseLevels[kind];
    if (level>=3) return L"집·농장·상점·동료 거점을 모두 3단계까지 확장했습니다.";
    const int wood=3+level*2, ore=2+level;
    if (s.inventory.Count(ItemId::Hardwood)<wood || s.inventory.Count(ItemId::CopperOre)<ore)
        return L"거점 확장: 목재 3/5/7 + 구리 광석 2/3/4가 필요합니다.";
    s.inventory.Remove(ItemId::Hardwood,wood); s.inventory.Remove(ItemId::CopperOre,ore);
    ++s.baseLevels[kind]; s.nextBaseUpgrade=(kind+1)%4;
    return kind==0 ? L"집 확장 · 거점 휴식 회복 강화" : kind==1 ? L"농장 확장 · 재배 속도 증가" :
        kind==2 ? L"상점 확장 · 교역 개방" : L"동료 거점 확장 · 함께 사냥하는 동료 합류";
}
inline const wchar_t *HarvestFarm(GameState &s)
{
    if (!InventoryActions::AtStorage(s) || s.baseLevels[1]==0) return L"장로 옆에 농장을 먼저 만드세요.";
    const float interval=120.0f/s.baseLevels[1];
    if (s.farmTime<interval) return L"작물이 자라는 중입니다. 비가 오면 더 빨리 자랍니다.";
    if (!s.inventory.Add(ItemId::Herb,2)) return L"가방이 가득 찼습니다. 작물은 남아 있습니다.";
    s.farmTime-=interval; Record(s,1,2);
    return L"농장에서 약초 2개를 수확했습니다.";
}
inline const wchar_t *TradeOrFestival(GameState &s)
{
    if (!InventoryActions::AtStorage(s)) return L"교역·축제는 마을 거점에서 참여하세요.";
    if (s.baseLevels[2]==0) return L"상점 거점을 만들면 주화로 사냥 재료를 교역합니다.";
    if (s.coins<10 || !s.inventory.Add(ItemId::IronOre,2)) return L"주화 10개와 철광석 보관 공간이 필요합니다.";
    s.coins-=10;
    return L"주화 10개로 철광석 2개를 교역했습니다.";
}
inline void Approach(const WorldState &world,Vec2 &position,Vec2 target,float distance)
{
    const float length=std::hypot(target.x-position.x,target.z-position.z);
    if (length<.05f) return;
    const float step=std::min(distance,length);
    const Vec2 next={position.x+(target.x-position.x)/length*step,position.z+(target.z-position.z)/length*step};
    if (WorldLayout::CanTravel(world,position,next,.12f)) position=next;
    else
    {
        const Vec2 x={next.x,position.z},z={position.x,next.z};
        if (WorldLayout::CanTravel(world,position,x,.12f)) position=x;
        else if (WorldLayout::CanTravel(world,position,z,.12f)) position=z;
    }
}
inline void RewardKill(GameState &s,FieldMonster &field)
{
    if (field.monster.health>0 || field.rewardGranted) return;
    field.rewardGranted=true;
    QuestSystem::AddExperience(s,field.monster.definition.experience);
    Record(s,0);
    if (std::hypot(field.home.x-s.world.elder.x,field.home.z-s.world.elder.z)<40) Record(s,4);
}
inline void Update(GameState &s,float dt)
{
    s.worldTime+=dt;
    const auto *story=StorySystem::Current(s);
    const bool imbalance=story && (story->chapterId=="unbalanced" || story->chapterId=="exodus");
    if (imbalance) s.weather=s.player.x>480 ? 3 : 2;
    else if (s.endingChoice==1) s.weather=0;
    else s.weather=Season(s)==3 ? 4 : ((Day(s)+static_cast<int>(s.worldTime/120))%5==0 ? 2 : (Day(s)%3==1 ? 1 : 0));
    s.farmTime=std::min(1200.0f,s.farmTime+dt*(s.weather==1 ? 1.6f : s.weather==3 ? .5f : 1));
    s.arrowTime=std::max(0.0f,s.arrowTime-dt);
    s.companionCooldown=std::max(0.0f,s.companionCooldown-dt);
    if (s.baseLevels[0]>0 && InventoryActions::AtStorage(s))
        s.health=std::min(s.maxHealth,s.health+dt*s.baseLevels[0]);
    const auto &animalDefinitions=GameContent::Get().map.animals;
    for (size_t i=0;i<s.animals.size() && i<animalDefinitions.size();++i)
    {
        const auto origin=animalDefinitions[i].initial.home;
        const Vec2 destination={origin.x+(s.migrationChoice==2 ? 8 : 0),origin.z+(s.weather==3 ? -6 : 0)};
        if (!WorldLayout::IsBlocked(destination)) s.animals[i].home=destination;
    }
    const auto &regions=GameContent::Get().story.regions;
    for (size_t i=0;i<regions.size() && i<16;++i)
    {
        const auto &r=regions[i];
        if (s.player.x>=r.minimumX && s.player.x<=r.maximumX && s.player.z>=r.minimumZ && s.player.z<=r.maximumZ &&
            !(s.regionVisits & (1<<i))) { s.regionVisits|=1<<i; Record(s,2); }
    }
    if (s.baseLevels[3]>0)
    {
        if (std::hypot(s.companion.x-s.player.x,s.companion.z-s.player.z)>30)
        { s.companion=s.player; s.companionRoute={}; }
        if (std::hypot(s.companion.x-s.player.x,s.companion.z-s.player.z)>1.5f) Navigation::Move(s.world,s.companion,s.player,dt*3.8f,dt,s.companionRoute);
    }
    for (auto &field:s.fieldMonsters)
    {
        auto &monster=field.monster;
        if (field.collected)
        {
            field.respawnTime=std::max(0.0f,field.respawnTime-dt);
            if (field.respawnTime==0)
            {
                monster=MonsterSystem::Create(GameContent::Get().monsters.at(monster.definition.id),field.home);
                field.collected=field.rewardGranted=false;
            }
            continue;
        }
        if (!WorldStreaming::Active(s,field.home)) continue;
        MonsterSystem::UpdateTimers(monster,dt);
        if (monster.health<=0) continue;
        if (monster.attackCooldown<=0 && std::hypot(s.player.x-monster.position.x,s.player.z-monster.position.z)>5)
        {
            for (auto &npc:s.npcs)
                if (npc.health>0 && std::hypot(npc.position.x-monster.position.x,npc.position.z-monster.position.z)<2 &&
                    WorldLayout::ClearLine(monster.position,npc.position))
                { npc.health=std::max(0.0f,npc.health-monster.definition.damage*.25f); monster.attackCooldown=2; break; }
            if (monster.attackCooldown<=0)
                for (auto &other:s.fieldMonsters)
                    if (&other!=&field && other.monster.health>0 && other.monster.definition.id!=monster.definition.id &&
                        std::hypot(other.monster.position.x-monster.position.x,other.monster.position.z-monster.position.z)<2 &&
                        WorldLayout::ClearLine(monster.position,other.monster.position))
                    {
                        other.monster.health=std::max(0,other.monster.health-3); monster.attackCooldown=2;
                        if (other.monster.health==0) { other.rewardGranted=true; other.monster.deathTime=.01f; }
                        break;
                    }
        }
        const float health=s.health;
        MonsterSystem::UpdateWorldCombat(monster,s.world,s.player,s.dodge,dt,s.health,s.defense);
        if (s.health<health) { s.playerHitTime=.3f; s.playerSlowTime=.5f; }
        const float territory=std::hypot(monster.position.x-field.home.x,monster.position.z-field.home.z);
        if (territory>18 || (s.migrationChoice==2 && territory>8)) Approach(s.world,monster.position,field.home,dt*2);
        if (s.baseLevels[3]>0 && s.companionCooldown<=0 &&
            std::hypot(s.companion.x-monster.position.x,s.companion.z-monster.position.z)<3 &&
            WorldLayout::ClearLine(s.companion,monster.position))
        {
            if (MonsterSystem::Hit(monster,s.companion,2+s.baseLevels[3]*2).hit) s.companionCooldown=1;
            RewardKill(s,field);
        }
    }
}
}
