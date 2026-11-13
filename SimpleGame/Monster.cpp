#include "stdafx.h"
#include "GameConstants.h"
#include "Monster.h"
#include "Renderer.h"
#include "GameContent.h"
#include "Chapter1BoarAnimation.h"
#include "LootTable.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float HitFlashTime = GameConstants::Feedback::MonsterFlashDuration;

float Distance(Vec2 a, Vec2 b)
{
    const float x = a.x - b.x;
    const float z = a.z - b.z;
    return std::sqrt(x * x + z * z);
}

void DrawDrop(const MonsterState &monster, const MonsterDrawContext &context)
{
    const Vec2 item = {monster.position.x + 0.65f, monster.position.z - 0.25f};
    const Vec2 s = context.worldToScreen(item);
    const float glow = .55f + std::sin(context.time * 6.0f) * .18f;

    context.renderer->DrawSolidRect(s.x, s.z + 9, 34, 16, context.pi * .25f, .95f, .78f, .22f, .28f);
    context.renderer->DrawEffectRect(s.x - 7, s.z + 10, 18, 7, -.45f, .82f, .72f, .50f, 1, 4);
    context.renderer->DrawEffectRect(s.x + 7, s.z + 10, 18, 7, .45f, .82f, .72f, .50f, 1, 4);
    context.renderer->DrawSolidRect(s.x, s.z + 9, 7, 7, 0, 1.0f, .88f, .38f, glow);

    const auto label = L"전리품: " + LootTable::Summary(LootTable::Monster(monster.definition));
    context.renderer->DrawLabel(context.screenWidth * .5f + s.x, context.screenHeight * .5f + s.z + 75,
                                label.c_str(), 1.0f, .86f, .42f, .5f, 1, true);
}

void DrawInfo(const MonsterState &monster, const MonsterDrawContext &context, const Vec2 &s)
{
    const float healthRatio = std::max(0.0f, std::min(1.0f, static_cast<float>(monster.health) / monster.maxHealth));

    const float top = monster.definition.spriteHeight * (monster.definition.id == "ancient_guardian" ?
        GameConstants::Chatacabra::SpriteScale * .55f : .75f);
    context.renderer->DrawLabel(context.screenWidth * .5f + s.x - 76, context.screenHeight * .5f + s.z + top + 10,
                                monster.definition.name.c_str(), 1.0f, .86f, .62f);
    context.renderer->DrawSolidRect(s.x, s.z + top - 8, 132, 12, 0, .05f, .04f, .03f, .88f);
    context.renderer->DrawSolidRect(s.x - 66 + healthRatio * 66.0f, s.z + top - 8, 132.0f * healthRatio, 6, 0, .78f, .12f,
                                    .08f, 1);
    if (monster.definition.id == "ancient_guardian")
    {
        const wchar_t *status = monster.chatStagger > 0 ? L"돌 파괴 · 경직" :
            monster.chatPattern == 1 && monster.chatElapsed >= GameConstants::Chatacabra::SecondImpact ? L"혀 노출 · 피해 2배" :
            monster.chatPattern == 0 ? L"혀 공격" : monster.chatPattern == 1 ? L"연속 내려찍기" :
            monster.chatPattern == 2 ? L"돌 부착" : monster.chatArmorHits > 0 ? L"앞다리 강화" : L"";
        context.renderer->DrawLabel(context.screenWidth*.5f+s.x-76,context.screenHeight*.5f+s.z+top+36,
            status,1,.72f,.32f,.8f);
    }
}

void DrawBoar(const MonsterState &monster, const MonsterDrawContext &context)
{
    const Vec2 s = context.worldToScreen(monster.position);
    const bool alive = monster.health > 0;
    context.drawSprite(monster.position, 1, 2, 128, 96, true);
    if (alive && monster.definition.id != "ancient_guardian")
    {
        const float glow = .55f + std::sin(context.time * 8.0f) * .18f;
        context.renderer->DrawSolidRect(s.x + 25, s.z + 28, 14, 8, 0, .95f, .16f, .04f, glow);
    }
}

void DrawHitEffect(const MonsterState &monster, const MonsterDrawContext &context, const Vec2 &s)
{
    if (monster.hitFlash <= 0)
    {
        return;
    }

    const float alpha = monster.hitFlash / HitFlashTime;
    context.renderer->DrawEffectRect(s.x, s.z + 24, 55, 37, -.5f, 1, .67f, .28f, .7f * alpha, 15);

    for (int i = 0; i < 5; ++i)
    {
        const float angle = i * 1.26f;
        const float travel = (1.0f - alpha) * 35;
        context.renderer->DrawEffectRect(s.x + std::cos(angle) * (9 + travel),
                                        s.z + 22 + std::sin(angle) * (5 + travel * .5f),
                                        7 * alpha + 1, 7 * alpha + 1, angle, 1, .75f, .30f, alpha, 15);
    }
}

void DrawDeathEffect(const MonsterState &monster, const MonsterDrawContext &context, const Vec2 &s)
{
    if (monster.deathTime <= 0)
    {
        return;
    }

    const float t = std::min(monster.deathTime, 1.0f);
    const float alpha = std::max(0.0f, 1.0f - t);

    for (int i = 0; i < 6; ++i)
    {
        const float angle = i * 1.05f;
        context.renderer->DrawSolidRect(s.x + std::cos(angle) * (18.0f + t * 34.0f),
                                        s.z + 8 + std::sin(angle) * (6.0f + t * 18.0f), 16 + t * 18.0f, 8,
                                        context.pi * .25f, .38f, .28f, .16f, alpha * .42f);
    }
}
} // namespace

namespace MonsterSystem
{
MonsterState CreateEncounter()
{
    const auto &content = GameContent::Get();
    return Create(content.monsters.at(content.map.monsterId),content.map.monsterSpawn);
}

MonsterState Create(const MonsterDefinition &definition, Vec2 position)
{
    MonsterState monster = {};
    monster.definition = definition;
    monster.position = position;
    monster.maxHealth = monster.health = monster.definition.health;
    return monster;
}

bool IsAlive(const MonsterState &monster)
{
    return monster.health > 0;
}

bool IsInteractableDrop(const MonsterState &monster, Vec2 player)
{
    return !IsAlive(monster) && Distance(player, monster.position) < monster.definition.dropRange;
}

MonsterHitResult Hit(MonsterState &monster, Vec2 player, int attackPower)
{
    if (!IsAlive(monster) || Distance(player, monster.position) >= monster.definition.hitRange)
    {
        return {false, false};
    }

    const bool committedChatDirection = monster.chatPattern >= 0;
    if (monster.definition.id == "ancient_guardian")
    {
        if (monster.chatPattern == 1 && monster.chatElapsed >= GameConstants::Chatacabra::SecondImpact)
            attackPower *= GameConstants::Chatacabra::WeakDamageMultiplier;
        if (monster.chatArmorHits > 0 && --monster.chatArmorHits == 0)
        {
            monster.chatStagger = GameConstants::Chatacabra::StaggerDuration;
            monster.chatPattern = -1;
            monster.attackCooldown = monster.chatStagger;
            monster.chatImpactTime = GameConstants::Chatacabra::ImpactDuration;
            monster.chatImpactPosition = monster.position;
            monster.chatImpactType = 1;
        }
    }
    monster.health = std::max(0, monster.health - attackPower);
    monster.hitFlash = HitFlashTime;
    if (monster.definition.id == "rampaging_boar")
    {
        monster.hitAnimation = Chapter1BoarAnimation::Duration(Chapter1BoarAnimation::Hit);
        monster.attackAnimation = 0.0f;
        if (Distance(player, monster.position) > .001f)
            monster.animationFacing = {player.x - monster.position.x, player.z - monster.position.z};
    }

    if (monster.health > 0)
    {
        return {true, false};
    }

    if ((monster.definition.id != "ancient_guardian" || !committedChatDirection) &&
        Distance(player,monster.position) > .001f)
        monster.animationFacing = {player.x-monster.position.x,player.z-monster.position.z};
    monster.deathTime = .01f;
    return {true, true};
}

void UpdateTimers(MonsterState &monster, float deltaTime)
{
    monster.attackCooldown = std::max(0.0f, monster.attackCooldown - deltaTime);
    monster.hitFlash = std::max(0.0f, monster.hitFlash - deltaTime);
    monster.attackAnimation = std::max(0.0f, monster.attackAnimation - deltaTime);
    monster.hitAnimation = std::max(0.0f, monster.hitAnimation - deltaTime);
    monster.chatStagger = std::max(0.0f, monster.chatStagger - deltaTime);
    monster.chatImpactTime = std::max(0.0f, monster.chatImpactTime - deltaTime);

    if (monster.deathTime > 0)
    {
        monster.deathTime += deltaTime;
    }
}

bool UpdateChatacabra(MonsterState &monster, Vec2 player, float dodgeTime, float dt, float &health, int defense)
{
    using namespace GameConstants::Chatacabra;
    if (monster.chatStagger > 0) return false;
    const float distance=Distance(player,monster.position);
    if (monster.chatPattern < 0)
    {
        if (distance > EngageRange && distance < monster.definition.chaseRange)
        {
            monster.position.x+=(player.x-monster.position.x)/distance*monster.definition.speed*dt;
            monster.position.z+=(player.z-monster.position.z)/distance*monster.definition.speed*dt;
        }
        else if (distance <= EngageRange && monster.attackCooldown <= 0)
        {
            monster.chatPattern=monster.chatNext;
            monster.chatNext=(monster.chatNext+1)%3;
            monster.chatElapsed=0; monster.chatHitMask=0;
            monster.chatTarget=player;
            if (distance > .001f) monster.animationFacing={player.x-monster.position.x,player.z-monster.position.z};
        }
        return false;
    }
    monster.chatElapsed+=dt;
    if (monster.chatPattern==2)
    {
        if (monster.chatElapsed>=CoatTime && monster.chatHitMask==0)
        {
            monster.chatArmorHits=ArmorBreakHits; monster.chatHitMask=1;
            monster.chatImpactTime=ImpactDuration; monster.chatImpactPosition=monster.position; monster.chatImpactType=1;
        }
    }
    bool damaged=false;
    const int impacts=monster.chatPattern==1 ? 2 : (monster.chatPattern==0 ? 1 : 0);
    for (int event=0;event<impacts;++event)
    {
        if (monster.chatElapsed<(event==0 ? FirstImpact : SecondImpact) || (monster.chatHitMask&(1<<event))) continue;
        monster.chatHitMask|=1<<event;
        monster.chatImpactTime=ImpactDuration; monster.chatImpactType=monster.chatPattern;
        monster.chatImpactPosition=monster.chatTarget;
        bool inRange=false;
        if (monster.chatPattern==0)
        {
            const float length=std::sqrt(monster.animationFacing.x*monster.animationFacing.x+monster.animationFacing.z*monster.animationFacing.z);
            const float x=player.x-monster.position.x,z=player.z-monster.position.z;
            const float forward=(x*monster.animationFacing.x+z*monster.animationFacing.z)/std::max(.001f,length);
            const float side=std::abs(x*monster.animationFacing.z-z*monster.animationFacing.x)/std::max(.001f,length);
            inRange=forward>=0 && forward<=TongueRange && side<=TongueHalfWidth;
        }
        else inRange=Distance(player,monster.chatTarget)<=(monster.chatArmorHits>0 ? ArmoredSlamRadius : SlamRadius);
        if (inRange && dodgeTime<=0 && monster.definition.damage>0)
        {
            health-=std::max(1.0f,monster.definition.damage*(monster.chatArmorHits>0 ? ArmoredDamageMultiplier : 1.0f)-std::max(0,defense));
            damaged=true;
        }
    }
    const float duration=monster.chatPattern==0 ? TongueDuration : (monster.chatPattern==1 ? SlamDuration : CoatDuration);
    if (monster.chatElapsed>=duration) { monster.chatPattern=-1; monster.attackCooldown=Recovery; }
    return damaged;
}

bool UpdateChaseAndAttack(MonsterState &monster, Vec2 player, float dodgeTime, float deltaTime, float &playerHealth, int defense)
{
    if (!IsAlive(monster))
    {
        return false;
    }

    const float distance = Distance(player, monster.position);
    if (monster.definition.id == "ancient_guardian")
        return UpdateChatacabra(monster,player,dodgeTime,std::max(0.0f,deltaTime),playerHealth,defense);

    if (monster.definition.id == "rampaging_boar" && distance > .001f &&
        monster.attackAnimation <= 0.0f && monster.hitAnimation <= 0.0f)
        monster.animationFacing = {player.x - monster.position.x, player.z - monster.position.z};

    if (distance < monster.definition.chaseRange && distance > monster.definition.attackRange)
    {
        monster.position.x += (player.x - monster.position.x) / distance * monster.definition.speed * deltaTime;
        monster.position.z += (player.z - monster.position.z) / distance * monster.definition.speed * deltaTime;
        return false;
    }

    if (distance <= monster.definition.attackRange && monster.attackCooldown <= 0 && dodgeTime <= 0)
    {
        playerHealth -= monster.definition.damage <= 0 ? 0 : std::max(1.0f,monster.definition.damage-std::max(0,defense));
        monster.attackCooldown = monster.definition.cooldown;
        if (monster.definition.id == "rampaging_boar")
        {
            monster.attackAnimation = Chapter1BoarAnimation::Duration(Chapter1BoarAnimation::Attack);
        }
        return true;
    }

    return false;
}

void Draw(const MonsterState &monster, const MonsterDrawContext &context, bool showDrop)
{
    const Vec2 s = context.worldToScreen(monster.position);

    if (IsAlive(monster))
    {
        DrawBoar(monster, context);
        DrawHitEffect(monster, context, s);
        DrawInfo(monster, context, s);
        return;
    }

    DrawBoar(monster, context);
    DrawDeathEffect(monster, context, s);

    if (showDrop)
    {
        DrawDrop(monster, context);
    }
}
} // namespace MonsterSystem
