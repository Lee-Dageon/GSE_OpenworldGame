#pragma once
#include "GameConstants.h"

#include "Monster.h"
#include "Inventory.h"
#include "RegionalState.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>

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
    float health = 100;
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

struct FieldMonster
{
    MonsterState monster = {};
    Vec2 home = {};
    bool collected = false, rewardGranted = false;
    float respawnTime = 0;
};
struct EscortState { Vec2 position = {}; bool following=false, arrived=false; };
struct ContractState { bool active=false; int progress=0, completed=0; };
struct ChunkObject { int kind=0, index=0; };
struct LoadedChunk { std::vector<ChunkObject> objects; };

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
    Inventory storage = {{}};
    int inventorySelection = 0, inventorySort = 0;
    int equippedWeapon = -1, equippedArmor = -1, weaponStyle = 0;
    bool spearCrafted = false, bowCrafted = false;
    std::vector<FieldMonster> fieldMonsters;
    std::array<EscortState,2> escorts = {};
    std::array<ContractState,5> contracts = {};
    std::array<int,4> baseLevels = {};
    double worldTime=0;
    float farmTime=0, companionCooldown=0;
    int coins=0, festivalClaimDay=-1, regionVisits=0, nextBaseUpgrade=0;
    Vec2 companion = {};
    bool journalOpen=false, prologueSeen=false;
    float prologueTime=-1;
    bool prologueLossApplied=false;
    int lostWeaponTier=-1,lostArmorTier=-1;
    std::map<std::string,RegionRuntime> regions;
    std::map<std::string,RegionProgress> regionProgress;
    std::map<std::string,std::string> orphanRecords;
    int regionLoads=0,regionUnloads=0;
    bool trackingActive=false,variantResearchActive=false;
    int trackingStep=0,trackingCompleted=0,variantSamples=0,variantResearchCompleted=0;
    std::set<std::string> variantSurveyed;
    int festivalRound=-1,festivalStage=0,festivalScore=0,festivalFinished=-1;
    int worldEventStage=0,worldEventKills=0,worldEventCompleted=0;
    float worldEventTime=0;
    std::vector<FieldMonster> eventMonsters;
    int worldEventDamage=0;
    int weather=0;
    float arrowTime=0;
    Vec2 arrowFrom = {}, arrowTo = {};
    std::map<std::pair<int,int>,LoadedChunk> loadedChunks;
    int streamX=100000,streamZ=100000,chunkLoads=0,chunkUnloads=0;
    bool inventoryOpen = false, hunterBladeCrafted = false;
    int inventoryPage = 0;
    bool armorRecipeUnlocked = false, hunterArmorCrafted = false, storyFinished = false;
    bool ruinDeviceDisabled = false;
    int rescuedSurvivors = 0;
    // 0 undecided, 1 hunt/restore, 2 open migration route/shut down.
    int migrationChoice = 0, endingChoice = 0;
    int weaponTier = 0, armorTier = 0;
    float endingTime = 0;
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
    int lootField = -1;
    int completedHunts = 0;
    int level = 1, experience = 0, experienceToNextLevel = 20, attackPower = 2;
    QuestStage quest = QuestStage::Elder;
    std::wstring message;
};

GameState CreateInitialGameState();
