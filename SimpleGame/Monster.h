#pragma once

class Renderer;

struct Vec2
{
    float x;
    float z;
};

struct MonsterState
{
    Vec2 position;
    int maxHealth;
    int health;
    float attackCooldown;
    float hitFlash;
    float deathTime;
};

struct MonsterHitResult
{
    bool hit;
    bool defeated;
};

using WorldToScreenFunction = Vec2 (*)(Vec2);
using DrawSpriteFunction = void (*)(Vec2, int, int, float, float, bool);

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
MonsterState CreateLakeBoar();
bool IsAlive(const MonsterState &monster);
bool IsInteractableDrop(const MonsterState &monster, Vec2 player);
MonsterHitResult Hit(MonsterState &monster, Vec2 player, int attackPower);
void UpdateTimers(MonsterState &monster, float deltaTime);
bool UpdateChaseAndAttack(MonsterState &monster, Vec2 player, float dodgeTime, float deltaTime, float &playerHealth);
void Draw(const MonsterState &monster, const MonsterDrawContext &context, bool showDrop);
}
