#pragma once
#include "LivingWorld.h"

namespace AdventureEvents
{
inline const Vec2 TrackSites[]={{70,50},{110,220},{180,430}};
inline const Vec2 LanternSites[]={{0,-8},{5,-10},{10,-8}};
inline Vec2 FestivalTarget() { return {0,-12}; }
inline const wchar_t *StartTracking(GameState &s)
{
    if (s.trackingActive) return L"추적 의뢰 진행 중 · 남쪽 흔적, 숲 야영지, 유적 신호석을 차례로 E 조사";
    s.trackingActive=true; s.trackingStep=0;
    return L"반복 추적 의뢰 수락 · 남쪽 흔적 (70,50)부터 E로 조사하세요.";
}
inline const wchar_t *StartResearch(GameState &s)
{
    if (s.variantResearchActive) return L"서로 다른 변이종의 시체 두 곳을 E 조사하고 장로에게 보고하세요.";
    s.variantResearchActive=true; s.variantSamples=0; s.variantSurveyed.clear();
    return L"변종 조사 수락 · 사막 변이종 또는 고원의 폭풍 변종 두 개체를 조사하세요.";
}
inline void Inspect(GameState &s,const std::string &id,const std::string &monster)
{
    if (!s.variantResearchActive || (monster.find("variant")==std::string::npos && monster!="mutated_guardian")) return;
    if (s.variantSurveyed.insert(id).second) s.variantSamples=std::min(2,s.variantSamples+1);
}
inline const wchar_t *Report(GameState &s)
{
    if (!InventoryActions::AtStorage(s)) return L"추적·변종 조사·대형 사건은 장로 옆에서 보고하세요.";
    bool done=false;
    if (s.trackingActive && s.trackingStep==3)
    { s.trackingActive=false; ++s.trackingCompleted; s.coins+=35; QuestSystem::AddExperience(s,20); done=true; }
    if (s.variantResearchActive && s.variantSamples>=2)
    { s.variantResearchActive=false; ++s.variantResearchCompleted; s.coins+=50; QuestSystem::AddExperience(s,30); done=true; }
    if (s.worldEventStage==3)
    {
        if (s.worldEventKills>=2) { s.coins+=60; QuestSystem::AddExperience(s,40); }
        ++s.worldEventCompleted; s.worldEventStage=0; s.worldEventTime=0; s.eventMonsters.clear(); done=true;
    }
    return done ? L"추적·변종 조사·사건 결과를 보고하고 보상을 받았습니다." : nullptr;
}
inline const wchar_t *Festival(GameState &s)
{
    if (!InventoryActions::AtStorage(s)) return L"축제는 장로 옆 일지의 4로 참여하세요.";
    s.festivalFinished=std::max(s.festivalFinished,s.festivalClaimDay);
    if (s.festivalStage==0)
    {
        if (s.sprint && s.festivalFinished>=0)
        {
            s.festivalRound=s.festivalRound<s.festivalFinished ? s.festivalRound+1 : 0;
            s.festivalStage=1; s.festivalScore=0;
            return L"완료한 축제 재연 · 참가 재료는 무료이며 완료 보상을 중복 지급하지 않습니다.";
        }
        if (s.festivalFinished>=LivingWorld::Day(s)/7) return LivingWorld::TradeOrFestival(s);
        s.festivalRound=s.festivalFinished+1; s.festivalStage=1; s.festivalScore=0;
        return L"지난 축제 재연 시작 · 산딸기 2 + 섬유 2를 준비하고 일지 4로 무대를 꾸미세요.";
    }
    if (s.festivalStage==1)
    {
        const bool replay=s.festivalRound<=s.festivalFinished;
        if (!replay && (s.inventory.Count(ItemId::WildBerry)<2 || s.inventory.Count(ItemId::PlantFiber)<2))
            return L"축제 장식에 산딸기 2 + 식물 섬유 2가 필요합니다.";
        if (!replay) { s.inventory.Remove(ItemId::WildBerry,2); s.inventory.Remove(ItemId::PlantFiber,2); }
        s.festivalStage=2;
        return L"세 등불을 밝혀 주세요. (0,-8), (5,-10), (10,-8)에서 E";
    }
    if (s.festivalStage<4) return L"등불 세 곳을 E로 밝힌 뒤 (0,-12)의 축제 허수아비를 세 번 명중하세요.";
    if (s.festivalRound<=s.festivalFinished)
    { s.festivalStage=0; s.festivalScore=0; return L"지난 축제 재연 완료 · 기록과 보유품은 그대로 유지했습니다."; }
    if (!s.inventory.Add(ItemId::Potion)) return L"물약 공간을 비우면 축제 보상을 받을 수 있습니다.";
    s.coins+=25; s.festivalClaimDay=s.festivalFinished=s.festivalRound; s.festivalStage=0; s.festivalScore=0;
    return L"축제를 마쳤습니다. 물약 1 + 주화 25 획득 · 지난 회차도 차례로 다시 참여 가능";
}
inline const wchar_t *Interact(GameState &s)
{
    if (s.trackingActive && s.trackingStep<3 &&
        std::hypot(s.player.x-TrackSites[s.trackingStep].x,s.player.z-TrackSites[s.trackingStep].z)<2)
    {
        ++s.trackingStep;
        return s.trackingStep==3 ? L"흔적을 끝까지 추적했습니다. 장로 옆 J/2로 보고하세요." :
            s.trackingStep==1 ? L"숲 야영지 (110,220)로 흔적이 이어집니다." : L"유적 신호석 (180,430)에서 마지막 흔적을 확인하세요.";
    }
    if (s.festivalStage==2)
        for (int i=0;i<3;++i)
            if (std::hypot(s.player.x-LanternSites[i].x,s.player.z-LanternSites[i].z)<2 && !(s.festivalScore & (1<<i)))
            {
                s.festivalScore|=1<<i;
                if (s.festivalScore==7) { s.festivalStage=3; s.festivalScore=0; return L"등불이 모두 켜졌습니다. 축제 허수아비를 세 번 명중하세요."; }
                return L"축제 등불을 밝혔습니다.";
            }
    return nullptr;
}
inline bool HitFestival(GameState &s,float range)
{
    if (s.festivalStage!=3 || std::hypot(s.player.x,s.player.z+12)>range || !WorldLayout::ClearLine(s.player,FestivalTarget())) return false;
    if (++s.festivalScore>=3) { s.festivalStage=4; s.message=L"축제 훈련 완료 · 장로 옆 J/4로 결과 보고"; s.messageTime=5; }
    return true;
}
inline const wchar_t *StartRaid(GameState &s)
{
    if (s.worldEventStage!=0) return L"마을 방어 사건이 진행 중입니다. 경보 뒤 나타나는 두 몬스터를 막으세요.";
    s.worldEventStage=1; s.worldEventTime=0; s.worldEventKills=s.worldEventDamage=0; s.eventMonsters.clear();
    return L"마을 방어 사건 재연 · 10초 뒤 습격 · 처치 뒤 장로에게 J/2로 보고";
}
inline void Update(GameState &s,float dt)
{
    if (!s.worldEventStage && LivingWorld::Day(s)/3>s.worldEventCompleted) StartRaid(s);
    if (s.worldEventStage==0 || s.worldEventStage==3) return;
    s.worldEventTime+=dt;
    if (s.worldEventStage==1 && s.worldEventTime>=10)
    {
        for (int i=0;i<2;++i)
        {
            FieldMonster f; f.home={s.world.elder.x+12+i*5.0f,s.world.elder.z+10};
            if (WorldLayout::IsBlocked(f.home)) f.home={s.world.elder.x-12-i*5.0f,s.world.elder.z+10};
            auto definition=GameContent::Get().monsters.at("forest_boar"); definition.chaseRange=40;
            f.monster=MonsterSystem::Create(definition,f.home); s.eventMonsters.push_back(f);
        }
        s.worldEventStage=2; s.worldEventTime=0;
    }
    if (s.worldEventStage!=2) return;
    for (auto &f:s.eventMonsters)
    {
        if (f.monster.health<=0) continue;
        MonsterSystem::UpdateTimers(f.monster,dt);
        MonsterSystem::UpdateWorldCombat(f.monster,s.world,s.player,s.dodge,dt,s.health,s.defense);
    }
    if (s.worldEventKills>=2) { s.worldEventStage=3; s.message=L"마을 방어 성공 · 장로에게 J/2로 보고"; s.messageTime=6; }
    else if (s.worldEventTime>=120)
    {
        for (auto &npc:s.npcs) if (npc.identity>0) { npc.health=std::max(10.0f,npc.health-25); ++s.worldEventDamage; }
        s.worldEventStage=3; s.message=L"방어 시간이 지났습니다. 주민들이 부상당했습니다. 치료하고 사건을 보고하세요."; s.messageTime=7;
    }
}
inline bool AttackRaid(GameState &s,float range,int power,Vec2 &where,int &damage)
{
    FieldMonster *target=nullptr; float nearest=range;
    for (auto &f:s.eventMonsters)
    {
        const float distance=std::hypot(s.player.x-f.monster.position.x,s.player.z-f.monster.position.z);
        if (f.monster.health>0 && distance<nearest && WorldLayout::ClearLine(s.player,f.monster.position)) { nearest=distance; target=&f; }
    }
    if (!target) return false;
    const float old=target->monster.definition.hitRange; target->monster.definition.hitRange=range;
    const int health=target->monster.health; MonsterSystem::Hit(target->monster,s.player,power); target->monster.definition.hitRange=old;
    where=target->monster.position; damage=health-target->monster.health;
    if (target->monster.health==0 && !target->rewardGranted)
    { target->rewardGranted=true; ++s.worldEventKills; LivingWorld::Record(s,4); }
    return true;
}
}
