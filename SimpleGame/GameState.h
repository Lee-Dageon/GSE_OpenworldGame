#pragma once
#include "GameConstants.h"

#include "Monster.h"
#include "Inventory.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

enum class QuestStage
{
    Elder,
    Tracks,
    Herb,
    Hunt,
    Trophy,
    Return,
    Complete
};

struct NpcState
{
    Vec2 position;
    int column;
    int row;
    float phase;
    Vec2 home = {}, target = {}, facing = {0.0f, -1.0f};
    float wait = 0.0f;
    std::uint32_t wanderSeed = 0;
    bool moving = false;
    int identity = -1, conversations = 0;
};
struct AnimalState
{
    Vec2 position;
    Vec2 home;
    int column;
    int row;
    float phase;
    float speed;
    int health = GameConstants::World::AnimalHealth;
    float hitFlash = 0.0f, respawnTime = 0.0f;
    float deathTime = 0.0f;
    float fleeTime = 0.0f, hitProtection = 0.0f;
    Vec2 facing = {0,-1};
    bool scared = false;
    bool collected = false;
};

struct WorldState
{
    Vec2 spawn = {0.0f, -2.5f};
    Vec2 elder = {0.0f, 1.4f};
    Vec2 tracks = {13.0f, 13.0f};
    Vec2 herb = {25.0f, 23.0f};
    Vec2 lake = {45.0f, 39.0f};
    float lakeRadius = 12.0f;
    float minimumCoordinate = -65.0f;
    float maximumCoordinate = 85.0f;
};

struct DamageNumber
{
    Vec2 position = {};
    int damage = 0, combo = 0;
    float remaining = GameConstants::HitEffect::NumberDuration;
};

struct GatherNodeState
{
    Vec2 position = {};
    int kind = 0;
    float regrowTime = 0;
};

struct GameState
{
    int width = 1280, height = 720, lastTime = 0;
    std::array<bool, 256> keys = {};
    bool sprint = false, moving = false;
    Vec2 player = {0.0f, -2.5f}, facing = {0.707f, 0.707f};
    MonsterState monster = {};
    std::vector<NpcState> npcs;
    std::vector<AnimalState> animals;
    std::vector<GatherNodeState> gatherNodes;
    WorldState world;
    float cameraZoom = 1.0f;
    float bossIntroTime = -1.0f;
    bool bossDefeatIntro = false;
    bool bossIntroPending = false;
    float health = 100.0f, maxHealth = 100.0f, dodge = 0.0f, attack = 0.0f, messageTime = 7.0f, time = 0.0f;
    Inventory inventory;
    bool inventoryOpen = false, hunterBladeCrafted = false;
    int inventoryPage = 0;
    bool armorRecipeUnlocked = false, hunterArmorCrafted = false, storyFinished = false;
    bool ruinDeviceDisabled = false;
    float herbRegrowTime = 0.0f;
    int defense = 0, storyStep = -1;
    float playerHitTime = 0.0f, hitStop = 0.0f;
    float playerSlowTime = 0.0f;
    Vec2 playerHitPosition = {};
    std::vector<DamageNumber> damageNumbers;
    float attackHitTime = 0.0f, comboTime = 0.0f;
    int combo = 0;
    float lootTime = 0.0f, lootPickupTime = 0.0f;
    int lootAnimal = -1;
    int lootNode = -1;
    int completedHunts = 0;
    int level = 1, experience = 0, experienceToNextLevel = 20, attackPower = 2;
    QuestStage quest = QuestStage::Elder;
    std::wstring message;
};

GameState CreateInitialGameState();
