#pragma once

#include <functional>
#include <string>

class Renderer;

struct Vec2
{
    float x;
    float z;
};

struct MonsterDefinition
{
    std::string id;
    std::wstring name;
    int health = 0;
    float hitRange = 0, dropRange = 0, chaseRange = 0, attackRange = 0;
    float speed = 0, damage = 0, cooldown = 0;
    int experience = 0, hideDrop = 0, tuskDrop = 0;
    float spriteHeight = 125;
};

struct MonsterState
{
    Vec2 position;
    int maxHealth;
    int health;
    float attackCooldown;
    float hitFlash;
    float deathTime;
    MonsterDefinition definition;
    float attackAnimation = 0.0f, hitAnimation = 0.0f;
    Vec2 animationFacing = {1.0f, 0.0f};
    int chatPattern = -1, chatHitMask = 0, chatArmorHits = 0, chatImpactType = 0, chatAttacks = 0;
    float chatElapsed = 0, chatStagger = 0, chatImpactTime = 0;
    float chatWeakTime = 0, chatArmorCooldown = 0, chatRageTime = 0, chatArmorBreakTime = 0;
    bool chatRage = false, chatCombo = false;
    Vec2 chatTarget = {}, chatImpactPosition = {}, chatOrigin = {};
};

struct MonsterHitResult
{
    bool hit;
    bool defeated;
};

using WorldToScreenFunction = std::function<Vec2(Vec2)>;
using DrawSpriteFunction = std::function<void(Vec2, int, int, float, float, bool)>;

struct MonsterDrawContext
{
    Renderer *renderer;
    WorldToScreenFunction worldToScreen;
    DrawSpriteFunction drawSprite;
    int screenWidth;
    int screenHeight;
    float time;
    float pi;
};

namespace MonsterSystem
{
enum ChatPattern { Tongue, Slam, Coat, Charge, Roar };
Vec2 ChatDirection(const MonsterState &monster);
Vec2 ChatWeakPoint(const MonsterState &monster);
Vec2 ChatArmPoint(const MonsterState &monster, int side);
MonsterState CreateEncounter();
MonsterState Create(const MonsterDefinition &definition, Vec2 position);
bool IsAlive(const MonsterState &monster);
bool IsInteractableDrop(const MonsterState &monster, Vec2 player);
MonsterHitResult Hit(MonsterState &monster, Vec2 player, int attackPower);
void UpdateTimers(MonsterState &monster, float deltaTime);
bool UpdateChaseAndAttack(MonsterState &monster, Vec2 player, float dodgeTime, float deltaTime, float &playerHealth, int defense = 0);
void Draw(const MonsterState &monster, const MonsterDrawContext &context, bool showDrop);
} // namespace MonsterSystem
