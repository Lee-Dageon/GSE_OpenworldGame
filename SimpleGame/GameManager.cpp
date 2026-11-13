#include "stdafx.h"
#include "GameConstants.h"
#include "GameManager.h"

#include "Monster.h"
#include "PlayerController.h"
#include "QuestSystem.h"
#include "CraftingSystem.h"
#include "GameContent.h"
#include "StorySystem.h"
#include "WorldLayout.h"
#include "DamageFeedback.h"
#include "NpcSystem.h"
#include "LootTable.h"
#include "BossIntro.h"
#include "HarvestSystem.h"
#include "WildlifeSystem.h"

#include <algorithm>
#include <cmath>

GameManager::GameManager(GameState &state) : m_State(state) {}

void GameManager::BeginFrame(float deltaTime)
{
    m_State.time += deltaTime;
    m_State.dodge = std::max(0.0f, m_State.dodge - deltaTime);
    m_State.attack = std::max(0.0f, m_State.attack - deltaTime);
    m_State.playerSlowTime = std::max(0.0f, m_State.playerSlowTime - deltaTime);
    m_State.messageTime = std::max(0.0f, m_State.messageTime - deltaTime);
    MonsterSystem::UpdateTimers(m_State.monster, deltaTime);
    HarvestSystem::Update(m_State, deltaTime);
}

void GameManager::UpdateNpcs(float deltaTime)
{
    if (deltaTime <= 0.0f)
        return;
    const float dt = std::min(deltaTime, GameConstants::Timing::MaxDeltaTime);
    const auto distance = [](Vec2 a, Vec2 b) {
        return std::sqrt((a.x-b.x)*(a.x-b.x) + (a.z-b.z)*(a.z-b.z));
    };
    for (size_t index = 0; index < m_State.npcs.size(); ++index)
    {
        NpcState &npc = m_State.npcs[index];
        if (npc.wanderSeed == 0)
        {
            npc.home = npc.target = npc.position;
            npc.wanderSeed = static_cast<std::uint32_t>(index + 1) * 747796405u;
            npc.wait = .4f + static_cast<float>(index % 5) * .3f;
        }
        npc.moving = false;
        if (distance(npc.home, m_State.world.elder) < .1f)
            continue;
        const auto random = [&npc]() {
            npc.wanderSeed ^= npc.wanderSeed << 13;
            npc.wanderSeed ^= npc.wanderSeed >> 17;
            npc.wanderSeed ^= npc.wanderSeed << 5;
            return (npc.wanderSeed & 65535u) / 65535.0f;
        };
        const auto valid = [&](Vec2 p) {
            return p.x > m_State.world.minimumCoordinate + .25f &&
                   p.z > m_State.world.minimumCoordinate + .25f &&
                   p.x < m_State.world.maximumCoordinate - .25f &&
                   p.z < m_State.world.maximumCoordinate - .25f &&
                   distance(p, m_State.world.lake) > m_State.world.lakeRadius + .35f &&
                   distance(p, m_State.world.elder) > .9f && !WorldLayout::IsBlocked(p);
        };
        const float playerDistance = distance(npc.position, m_State.player);
        if (playerDistance < 1.6f)
        {
            if (playerDistance > .001f)
                npc.facing = {(m_State.player.x-npc.position.x)/playerDistance,
                              (m_State.player.z-npc.position.z)/playerDistance};
            continue;
        }
        npc.wait = std::max(0.0f, npc.wait - dt);
        if (npc.wait > 0.0f)
            continue;
        if (distance(npc.position, npc.target) < .06f)
        {
            for (int attempt = 0; attempt < 12; ++attempt)
            {
                const float angle = random() * 6.2831853f, radius = .75f + random() * 2.25f;
                const Vec2 target = {npc.home.x + std::cos(angle)*radius, npc.home.z + std::sin(angle)*radius};
                if (valid(target)) { npc.target = target; break; }
            }
        }
        const float remaining = distance(npc.position, npc.target);
        if (remaining < .06f)
        {
            npc.wait = .5f;
            continue;
        }
        npc.facing = {(npc.target.x-npc.position.x)/remaining, (npc.target.z-npc.position.z)/remaining};
        const float step = std::min(remaining, (.65f + (index % 4)*.1f) * dt);
        const Vec2 next = {npc.position.x + npc.facing.x*step, npc.position.z + npc.facing.z*step};
        if (!valid(next))
        {
            npc.target = npc.position;
            npc.wait = .4f + random();
            continue;
        }
        npc.position = next;
        npc.moving = true;
        npc.phase += dt * 8.0f;
        if (remaining <= step + .06f)
        {
            npc.target = npc.position;
            npc.wait = .6f + random() * 2.0f;
        }
    }
}

void GameManager::RecordHit(Vec2 position, int damage)
{
    if (damage <= 0) return;
    m_State.combo = m_State.comboTime > 0 ? m_State.combo + 1 : 1;
    m_State.comboTime = GameConstants::Combo::Window;
    m_State.attackHitTime = DamageFeedback::Duration;
    m_State.hitStop = std::max(m_State.hitStop, m_State.combo % GameConstants::Combo::HeavyInterval == 0 ?
        GameConstants::Feedback::HeavyHitStop : GameConstants::Feedback::AttackHitStop);
    if (m_State.damageNumbers.size() >= GameConstants::HitEffect::MaxNumbers) m_State.damageNumbers.erase(m_State.damageNumbers.begin());
    m_State.damageNumbers.push_back({position, damage, m_State.combo, GameConstants::HitEffect::NumberDuration});
}

void GameManager::UpdateGame(float deltaTime)
{
    if (m_State.inventoryOpen)
    {
        m_State.moving = false;
        return;
    }
    deltaTime = std::max(0.0f, deltaTime);
    BossIntro::TryStartVisible(m_State);
    if (BossIntro::Active(m_State))
    {
        m_State.moving = false;
        BossIntro::Update(m_State, deltaTime);
        return;
    }
    m_State.playerHitTime = std::max(0.0f, m_State.playerHitTime - deltaTime);
    m_State.attackHitTime = std::max(0.0f, m_State.attackHitTime - deltaTime);
    const float frozen = std::min(deltaTime, m_State.hitStop);
    m_State.hitStop -= frozen;
    deltaTime -= frozen;
    if (deltaTime <= 0.0f && (frozen > 0.0f || m_State.hitStop > 0.0f)) return;
    BeginFrame(deltaTime);
    m_State.herbRegrowTime = std::max(0.0f, m_State.herbRegrowTime - deltaTime);
    m_State.lootPickupTime = std::max(0.0f, m_State.lootPickupTime - deltaTime);
    if (m_State.lootTime > 0)
    {
        m_State.moving = false;
        m_State.lootTime = std::max(0.0f, m_State.lootTime - deltaTime);
        if (m_State.lootTime == 0 && m_State.lootNode >= 0)
        {
            ShowMessage(HarvestSystem::Complete(m_State).c_str());
        }
        else if (m_State.lootTime == 0 && m_State.lootAnimal >= 0)
        {
            const size_t index = static_cast<size_t>(m_State.lootAnimal);
            m_State.lootAnimal = -1;
            if (index >= m_State.animals.size()) return;
            auto &animal = m_State.animals[index];
            const float x = animal.position.x - m_State.player.x, z = animal.position.z - m_State.player.z;
            if (animal.health > 0 || animal.collected || x*x+z*z >= GameConstants::World::AnimalLootRange*GameConstants::World::AnimalLootRange) return;
            const auto drops = LootTable::Animal(animal.column);
            if (!LootTable::Grant(m_State.inventory, drops))
            { ShowMessage(L"가방이 가득 찼습니다. 전리품은 남아 있습니다."); return; }
            animal.collected = true;
            animal.respawnTime = GameConstants::World::AnimalRespawnSeconds;
            m_State.lootPickupTime = GameConstants::Loot::PickupDuration;
            ShowMessage((LootTable::Summary(drops) + L" 획득!").c_str());
        }
        else if (m_State.lootTime == 0 && MonsterSystem::IsInteractableDrop(m_State.monster, m_State.player))
        {
            const int hunts = m_State.completedHunts;
            if (StorySystem::HasDrop(m_State))
            {
                if (const wchar_t *message = StorySystem::Interact(m_State)) ShowMessage(message);
            }
            else if (m_State.quest == QuestStage::Trophy)
            {
                const auto result = QuestSystem::TryInteract(m_State, m_State.world.elder, m_State.world.tracks, m_State.world.herb);
                if (result == QuestSystem::InteractionResult::TrophyCollected)
                    ShowMessage((LootTable::Summary(LootTable::Monster(m_State.monster.definition)) + L" 획득!").c_str());
                else if (const wchar_t *message = QuestSystem::GetInteractionMessage(result)) ShowMessage(message);
            }
            if (m_State.completedHunts > hunts) m_State.lootPickupTime = GameConstants::Loot::PickupDuration;
        }
        return;
    }
    m_State.comboTime = std::max(0.0f, m_State.comboTime - deltaTime);
    if (m_State.comboTime <= 0) m_State.combo = 0;
    for (auto &number : m_State.damageNumbers) number.remaining -= deltaTime;
    m_State.damageNumbers.erase(std::remove_if(m_State.damageNumbers.begin(), m_State.damageNumbers.end(),
        [](const DamageNumber &number) { return number.remaining <= 0; }), m_State.damageNumbers.end());
    PlayerController::UpdateMovement(m_State, deltaTime);
    UpdateNpcs(deltaTime);
    StorySystem::Update(m_State);
    BossIntro::TryStartVisible(m_State);
    if (BossIntro::Active(m_State)) { m_State.moving = false; return; }
    for (AnimalState &animal : m_State.animals)
    {
        WildlifeSystem::Update(animal,m_State,deltaTime);
    }
    const float previousHealth = m_State.health;
    if ((m_State.quest == QuestStage::Hunt || StorySystem::IsHunting(m_State)) &&
        !m_State.bossIntroPending && MonsterSystem::UpdateChaseAndAttack(m_State.monster, m_State.player, m_State.dodge, deltaTime, m_State.health, m_State.defense))
    {
        if (m_State.health < previousHealth)
        {
            m_State.playerHitTime = DamageFeedback::Duration;
            m_State.playerSlowTime = GameConstants::Player::HitSlowDuration;
            m_State.hitStop = DamageFeedback::HitStop;
            m_State.playerHitPosition = m_State.player;
            m_State.combo = 0;
            m_State.comboTime = 0;
        }
        const auto message = m_State.monster.definition.name + L"에게 공격받았습니다. 공백 회피 · Q 회복";
        ShowMessage(message.c_str());
    }
    if (m_State.health <= 0.0f)
    {
        m_State.player = m_State.world.spawn;
        m_State.health = m_State.maxHealth;
        m_State.playerSlowTime = 0;
        ShowMessage(L"구조대의 도움으로 마을에 돌아왔습니다.");
    }
}

void GameManager::HandleCommand(GameCommand command)
{
    if (BossIntro::Active(m_State)) return;
    if (command == GameCommand::ToggleInventory)
    {
        m_State.inventoryOpen = !m_State.inventoryOpen;
        m_State.moving = false;
        return;
    }
    if (m_State.hitStop > 0.0f) return;
    if (m_State.lootTime > 0.0f) return;
    if (command == GameCommand::CraftHunterBlade || command == GameCommand::CraftHealingPotion || command == GameCommand::CraftHunterArmor)
    {
        const auto recipe = command == GameCommand::CraftHunterArmor ? CraftingSystem::Recipe::HunterArmor :
            (command == GameCommand::CraftHunterBlade ? CraftingSystem::Recipe::HunterBlade : CraftingSystem::Recipe::HealingPotion);
        ShowMessage(CraftingSystem::TryCraft(m_State,recipe));
        StorySystem::Update(m_State);
        return;
    }
    if (m_State.inventoryOpen && command != GameCommand::UsePotion)
        return;
    if (command == GameCommand::Interact)
    {
        if ((m_State.quest == QuestStage::Trophy || StorySystem::HasDrop(m_State)) &&
            MonsterSystem::IsInteractableDrop(m_State.monster, m_State.player))
        {
            m_State.lootTime = GameConstants::Loot::Duration;
            m_State.lootAnimal = -1;
            m_State.moving = false;
            const Vec2 direction = {m_State.monster.position.x - m_State.player.x, m_State.monster.position.z - m_State.player.z};
            if (std::abs(direction.x) + std::abs(direction.z) > .001f) m_State.facing = direction;
            ShowMessage(L"전리품을 뒤지는 중...");
            return;
        }
        int closest = -1;
        float distance = GameConstants::World::AnimalLootRange * GameConstants::World::AnimalLootRange;
        for (size_t i=0; i<m_State.animals.size(); ++i)
        {
            const auto &animal = m_State.animals[i];
            const float x=animal.position.x-m_State.player.x, z=animal.position.z-m_State.player.z;
            if (animal.health <= 0 && !animal.collected && x*x+z*z < distance)
            { distance=x*x+z*z; closest=static_cast<int>(i); }
        }
        if (closest >= 0)
        {
            m_State.lootAnimal = closest;
            m_State.lootTime = GameConstants::Loot::Duration;
            m_State.moving = false;
            const auto p=m_State.animals[closest].position;
            m_State.facing = {p.x-m_State.player.x,p.z-m_State.player.z};
            ShowMessage(L"동물 전리품을 수집합니다.");
            return;
        }
        if (const int node = HarvestSystem::Nearby(m_State); node >= 0)
        {
            ShowMessage(HarvestSystem::Start(m_State, node).c_str());
            return;
        }
        const float herbX=m_State.player.x-m_State.world.herb.x, herbZ=m_State.player.z-m_State.world.herb.z;
        if (m_State.quest != QuestStage::Elder && m_State.quest != QuestStage::Tracks && m_State.quest != QuestStage::Herb &&
            herbX*herbX+herbZ*herbZ<GameConstants::Loot::HerbRange*GameConstants::Loot::HerbRange)
        {
            if (m_State.herbRegrowTime>0) ShowMessage(L"약초가 다시 자라는 중입니다.");
            else if (!m_State.inventory.Add(ItemId::Herb)) ShowMessage(L"약초 보관 한도에 도달했습니다.");
            else { m_State.herbRegrowTime=GameConstants::Loot::HerbRegrowSeconds; ShowMessage(L"약초 1개 획득! 야영지에서 물약을 제작할 수 있습니다."); }
            return;
        }
        if (const int npc=NpcSystem::Nearby(m_State); npc>=0)
        {
            const auto message=NpcSystem::Interact(m_State,m_State.npcs[npc]);
            ShowMessage(message.c_str());
            StorySystem::Update(m_State);
            return;
        }
        if (StorySystem::Current(m_State) || StorySystem::CanStart(m_State))
        {
            if (const wchar_t *message = StorySystem::Interact(m_State)) ShowMessage(message);
            return;
        }
        const QuestSystem::InteractionResult result =
            QuestSystem::TryInteract(m_State, m_State.world.elder, m_State.world.tracks, m_State.world.herb);
        if (const wchar_t *message = QuestSystem::GetInteractionMessage(result))
            ShowMessage(message);
        return;
    }
    if (command == GameCommand::UsePotion && PlayerController::TryUsePotion(m_State))
    {
        ShowMessage(L"회복 물약을 사용했습니다.");
        return;
    }
    if (command == GameCommand::Dodge)
    {
        PlayerController::StartDodge(m_State);
        return;
    }
    if (command == GameCommand::Attack && m_State.attack <= 0.0f)
    {
        m_State.attack = GameConstants::Player::AttackDuration;
        const bool activeMonster = (m_State.quest == QuestStage::Hunt || StorySystem::IsHunting(m_State)) &&
            m_State.monster.health > 0;
        const float mx=m_State.monster.position.x-m_State.player.x, mz=m_State.monster.position.z-m_State.player.z;
        float nearest = activeMonster ? std::min(GameConstants::World::AnimalHitRange*GameConstants::World::AnimalHitRange,
            mx*mx+mz*mz) : GameConstants::World::AnimalHitRange*GameConstants::World::AnimalHitRange;
        int animalIndex = -1;
        for (size_t i=0;i<m_State.animals.size();++i)
        {
            const auto &animal=m_State.animals[i];
            const float x=animal.position.x-m_State.player.x,z=animal.position.z-m_State.player.z;
            if (animal.health>0 && !animal.collected && x*x+z*z<nearest)
            { nearest=x*x+z*z; animalIndex=static_cast<int>(i); }
        }
        if (animalIndex>=0)
        {
            auto &animal=m_State.animals[animalIndex];
            const int damage=WildlifeSystem::Hit(animal,m_State.player,
                std::max(GameConstants::Player::MinimumDamage,m_State.attackPower/GameConstants::Player::DamageDivisor));
            if (damage==0) return;
            RecordHit(animal.position,damage);
            if (animal.health==0)
            { animal.deathTime = 0; QuestSystem::AddExperience(m_State,GameConstants::World::AnimalExperience); ShowMessage(L"동물을 사냥했습니다. E로 전리품을 수집하세요."); }
            return;
        }
        if (m_State.quest != QuestStage::Hunt && !StorySystem::IsHunting(m_State))
            return;
        const int healthBefore = m_State.monster.health;
        const MonsterHitResult result = MonsterSystem::Hit(m_State.monster, m_State.player, std::max(GameConstants::Player::MinimumDamage, m_State.attackPower / GameConstants::Player::DamageDivisor));
        const int damage = healthBefore - m_State.monster.health;
        if (result.hit && damage > 0)
        {
            RecordHit(m_State.monster.position, damage);
        }
        if (!result.defeated)
            return;
        if (m_State.monster.definition.id == "ancient_guardian") BossIntro::StartDefeat(m_State);
        if (StorySystem::IsHunting(m_State))
        {
            QuestSystem::AddExperience(m_State,m_State.monster.definition.experience);
            if (const wchar_t *message = StorySystem::EnemyDefeated(m_State)) ShowMessage(message);
            return;
        }
        m_State.quest = GameContent::Step(m_State.quest).next;
        ShowMessage(QuestSystem::AddExperience(m_State, m_State.monster.definition.experience) ? L"레벨이 올랐습니다! 최대 체력과 공격력이 증가했습니다."
                                                          : GameContent::Step(QuestStage::Hunt).message.c_str());
    }
}

const wchar_t *GameManager::GetHint() const
{
    if (m_State.lootNode >= 0) return L"자원을 채집하고 있습니다...";
    if ((m_State.quest == QuestStage::Trophy || StorySystem::HasDrop(m_State)) &&
        MonsterSystem::IsInteractableDrop(m_State.monster,m_State.player))
        return StorySystem::HasDrop(m_State) ? StorySystem::Hint(m_State) : QuestSystem::GetHint(m_State);
    const float herbX=m_State.player.x-m_State.world.herb.x, herbZ=m_State.player.z-m_State.world.herb.z;
    if (m_State.quest != QuestStage::Elder && m_State.quest != QuestStage::Tracks && m_State.quest != QuestStage::Herb &&
        herbX*herbX+herbZ*herbZ<GameConstants::Loot::HerbRange*GameConstants::Loot::HerbRange)
        return m_State.herbRegrowTime>0 ? L"약초 재생 중" : L"E  약초 채집";
    for (const auto &animal : m_State.animals)
    {
        const float x=animal.position.x-m_State.player.x,z=animal.position.z-m_State.player.z;
        if (animal.health<=0 && !animal.collected && x*x+z*z<GameConstants::World::AnimalLootRange*GameConstants::World::AnimalLootRange)
            return L"E  동물 전리품 수집";
    }
    if (const int node = HarvestSystem::Nearby(m_State); node >= 0)
        return HarvestSystem::IsOre(m_State.gatherNodes[node].kind) ? L"E  광맥 채굴 (3초)" : L"E  야생 재료 채집 (1초)";
    if (const int npc=NpcSystem::Nearby(m_State); npc>=0) return NpcSystem::Hint(m_State.npcs[npc]);
    if (StorySystem::Current(m_State) || StorySystem::CanStart(m_State)) return StorySystem::Hint(m_State);
    return QuestSystem::GetHint(m_State);
}

void GameManager::ShowMessage(const wchar_t *message)
{
    m_State.message = message;
    m_State.messageTime = 5.0f;
}

void GameManager::UpdateSceneGraph(Actor &sceneRoot, float deltaTime)
{
    sceneRoot.Update(deltaTime);
}
