#pragma once
#include "RegionContent.h"
#include "AdventureEvents.h"

namespace RegionSystem
{
inline const RegionGoal *Goal(const RegionRuntime &r,const RegionProgress &p)
{
    if (!p.started || p.finished) return nullptr;
    for (const auto &g:r.goals) if (g.id==p.stepId) return &g;
    return nullptr;
}
inline Vec2 Target(const RegionRuntime &r,const RegionGoal &g)
{
    for (const auto &s:r.sites) if (s.id==g.target) return s.position;
    for (const auto &e:r.enemies) if (e.id==g.target) return e.monster.position;
    return {};
}
inline void Snapshot(const RegionRuntime &r,RegionProgress &p)
{
    for (const auto &e:r.enemies)
        p.objects[e.id]={e.monster.position,e.monster.health,e.timer,e.collected,e.rewarded};
    for (const auto &g:r.gathers) p.objects[g.id]={g.position,-1,g.timer,false,false};
}
inline void Restore(RegionRuntime &r,const RegionProgress &p,float elapsed=0)
{
    for (auto &e:r.enemies)
    {
        const auto found=p.objects.find(e.id); if (found==p.objects.end()) continue;
        const auto &saved=found->second;
        e.monster.position=saved.position; e.monster.health=std::clamp(saved.health,0,e.monster.maxHealth);
        e.monster.attackCooldown=1; e.monster.deathTime=e.monster.health==0 ? 4.0f : 0;
        e.timer=std::max(0.0f,saved.timer-elapsed); e.collected=saved.collected; e.rewarded=saved.rewarded;
    }
    for (auto &g:r.gathers)
    { const auto found=p.objects.find(g.id); if (found!=p.objects.end()) g.timer=std::max(0.0f,found->second.timer-elapsed); }
}
inline void Stream(GameState &s,const std::string &directory="./Data/Regions")
{
    static std::map<std::string,std::vector<RegionContent::Descriptor>> catalogs;
    if (!catalogs.count(directory)) catalogs.emplace(directory,RegionContent::Catalog(directory));
    const auto &catalog=catalogs.at(directory);
    for (const auto &d:catalog)
    {
        const bool nearby=s.endingChoice!=0 && s.player.x>=d.minX-60 && s.player.x<=d.maxX+60 &&
            s.player.z>=d.minZ-60 && s.player.z<=d.maxZ+60;
        auto loaded=s.regions.find(d.id);
        if (nearby && loaded==s.regions.end())
        {
            auto region=RegionContent::Load(d,directory); // Only a complete validated payload becomes live.
            const auto saved=s.regionProgress.find(d.id);
            if (saved!=s.regionProgress.end()) Restore(region,saved->second,static_cast<float>(std::max(0.0,s.worldTime-saved->second.lastWorldTime)));
            s.regions.emplace(d.id,std::move(region)); ++s.regionLoads;
        }
        else if (!nearby && loaded!=s.regions.end())
        {
            Snapshot(loaded->second,s.regionProgress[d.id]); s.regionProgress[d.id].lastWorldTime=s.worldTime;
            s.regions.erase(loaded); ++s.regionUnloads;
        }
    }
}
inline const wchar_t *Advance(GameState &s,const RegionRuntime &r,RegionProgress &p,const RegionGoal &g)
{
    p.stepId=g.next; p.finished=g.next=="end";
    if (p.finished) { s.coins+=100; QuestSystem::AddExperience(s,80); }
    return g.message.c_str();
}
inline const wchar_t *Interact(GameState &s)
{
    for (auto &entry:s.regions)
    {
        auto &r=entry.second; auto &p=s.regionProgress[r.id];
        const auto *g=Goal(r,p);
        if (!p.started)
        {
            for (const auto &site:r.sites)
                if (site.kind==0 && std::hypot(s.player.x-site.position.x,s.player.z-site.position.z)<2)
                { p.started=true; p.stepId=r.goals.front().id; return L"눈안개 고원 독립 의뢰 수락 · 마을 기록에서 폭풍의 원인을 조사하세요."; }
        }
        if (g && std::hypot(s.player.x-Target(r,*g).x,s.player.z-Target(r,*g).z)<2.4f)
        {
            if (g->action=="interact" || g->action=="report") return Advance(s,r,p,*g);
            if (g->action=="craft")
            {
                if (s.inventory.Count(ItemId::StormShard)<2 || s.inventory.Count(ItemId::FrostHerb)<2)
                    return L"기후 안정 부적: 폭풍 파편 2 + 서리 약초 2가 필요합니다.";
                s.inventory.Remove(ItemId::StormShard,2); s.inventory.Remove(ItemId::FrostHerb,2); s.defense+=2;
                return Advance(s,r,p,*g);
            }
            if (g->action=="collect")
                for (auto &e:r.enemies)
                    if (e.id==g->target && e.monster.health<=0 && !e.collected)
                    {
                        if (!LootTable::Grant(s.inventory,LootTable::Monster(e.monster.definition))) return L"전리품 공간을 비워 주세요. 진행은 보존합니다.";
                        e.collected=true; e.timer=180; AdventureEvents::Inspect(s,r.id+"/"+e.id,e.monster.definition.id);
                        return Advance(s,r,p,*g);
                    }
        }
        for (auto &e:r.enemies)
            if (!e.collected && e.monster.health<=0 && std::hypot(s.player.x-e.monster.position.x,s.player.z-e.monster.position.z)<2)
            {
                if (g && e.id==g->target && g->action=="hunt") return L"현재 사냥 결과를 확인하는 중입니다.";
                if (!LootTable::Grant(s.inventory,LootTable::Monster(e.monster.definition))) return L"전리품 공간이 부족합니다.";
                e.collected=true; e.timer=180; AdventureEvents::Inspect(s,r.id+"/"+e.id,e.monster.definition.id);
                return L"고원 변종의 전리품과 조사 시료를 수집했습니다.";
            }
        for (auto &node:r.gathers)
            if (node.timer<=0 && std::hypot(s.player.x-node.position.x,s.player.z-node.position.z)<1.8f)
            {
                if (!s.inventory.Add(static_cast<ItemId>(node.item),node.amount)) return L"재료 보관 공간이 부족합니다.";
                node.timer=90; LivingWorld::Record(s,1,node.amount); return L"눈안개 고원의 고유 재료를 채집했습니다.";
            }
        for (const auto &site:r.sites)
            if (std::hypot(s.player.x-site.position.x,s.player.z-site.position.z)<2)
            {
                if (p.finished && site.kind==0) return L"고원은 안정을 되찾았습니다. 남은 변종을 조사하고 새 재료로 사냥을 이어가세요.";
                return site.dialogue.c_str();
            }
    }
    return nullptr;
}
inline void Update(GameState &s,float dt)
{
    Stream(s);
    for (auto &entry:s.regions)
    {
        auto &r=entry.second; auto &p=s.regionProgress[r.id]; const auto *g=Goal(r,p);
        if (s.player.x>=r.minX && s.player.x<=r.maxX && s.player.z>=r.minZ && s.player.z<=r.maxZ)
            s.weather=(p.finished || p.stepId=="frost_report") ? 4 : 2;
        for (auto &node:r.gathers) node.timer=std::max(0.0f,node.timer-dt);
        for (auto &e:r.enemies)
        {
            if (e.collected)
            {
                e.timer=std::max(0.0f,e.timer-dt);
                if (e.timer==0)
                { e.monster=MonsterSystem::Create(GameContent::Get().monsters.at(e.monster.definition.id),e.home); e.collected=e.rewarded=false; }
                continue;
            }
            MonsterSystem::UpdateTimers(e.monster,dt);
            const auto before=e.monster.position;
            MonsterSystem::UpdateChaseAndAttack(e.monster,s.player,s.dodge,dt,s.health,s.defense);
            if (!WorldLayout::CanMove(s.world,before,e.monster.position)) e.monster.position=before;
            if (std::hypot(e.monster.position.x-e.home.x,e.monster.position.z-e.home.z)>18)
                LivingWorld::Approach(s.world,e.monster.position,e.home,dt*2);
        }
        if (!g) continue;
        bool completed=g->action=="travel" && std::hypot(s.player.x-Target(r,*g).x,s.player.z-Target(r,*g).z)<2;
        if (g->action=="hunt")
            for (const auto &e:r.enemies) if (e.id==g->target && e.monster.health<=0) completed=true;
        if (completed) { s.message=Advance(s,r,p,*g); s.messageTime=7; }
    }
}
inline bool Attack(GameState &s,float range,int power,Vec2 &where,int &damage)
{
    RegionEnemy *target=nullptr; RegionProgress *progress=nullptr; float nearest=range;
    for (auto &entry:s.regions)
        for (auto &enemy:entry.second.enemies)
        {
            const float distance=std::hypot(s.player.x-enemy.monster.position.x,s.player.z-enemy.monster.position.z);
            if (enemy.monster.health>0 && distance<nearest && WorldLayout::ClearLine(s.player,enemy.monster.position))
            { nearest=distance; target=&enemy; progress=&s.regionProgress[entry.first]; }
        }
    if (!target) return false;
    const float old=target->monster.definition.hitRange; target->monster.definition.hitRange=range;
    const int health=target->monster.health; MonsterSystem::Hit(target->monster,s.player,power); target->monster.definition.hitRange=old;
    damage=health-target->monster.health; where=target->monster.position;
    if (target->monster.health==0 && !target->rewarded)
    {
        target->rewarded=true; ++progress->kills; QuestSystem::AddExperience(s,target->monster.definition.experience); LivingWorld::Record(s,0);
    }
    return true;
}
inline bool Blocked(const GameState &s,Vec2 p)
{
    for (const auto &entry:s.regions)
        for (const auto &site:entry.second.sites)
            if (site.kind==1 && std::hypot(p.x-site.position.x,p.z-site.position.z)<1.8f) return true;
    return false;
}
}
