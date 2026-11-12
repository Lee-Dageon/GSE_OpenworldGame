#include "stdafx.h"
#include "GameManager.h"

#include "Monster.h"
#include "PlayerController.h"
#include "QuestSystem.h"
#include "CraftingSystem.h"
#include "GameContent.h"
#include "StorySystem.h"
#include "WorldLayout.h"
#include "DamageFeedback.h"

#include <algorithm>
#include <cmath>

GameManager::GameManager(GameState &state) : m_State(state) {}

void GameManager::BeginFrame(float deltaTime)
{
    m_State.time += deltaTime;
    m_State.dodge = std::max(0.0f, m_State.dodge - deltaTime);
    m_State.attack = std::max(0.0f, m_State.attack - deltaTime);
    m_State.messageTime = std::max(0.0f, m_State.messageTime - deltaTime);
    MonsterSystem::UpdateTimers(m_State.monster, deltaTime);
}

void GameManager::UpdateNpcs(float deltaTime)
{
    if (deltaTime <= 0.0f)
        return;
    const float dt = std::min(deltaTime, .05f);
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

void GameManager::UpdateGame(float deltaTime)
{
    if (m_State.inventoryOpen)
    {
        m_State.moving = false;
        return;
    }
    deltaTime = std::max(0.0f, deltaTime);
    m_State.playerHitTime = std::max(0.0f, m_State.playerHitTime - deltaTime);
    m_State.attackHitTime = std::max(0.0f, m_State.attackHitTime - deltaTime);
    const float frozen = std::min(deltaTime, m_State.hitStop);
    m_State.hitStop -= frozen;
    deltaTime -= frozen;
    if (deltaTime <= 0.0f && (frozen > 0.0f || m_State.hitStop > 0.0f)) return;
    BeginFrame(deltaTime);
    m_State.lootPickupTime = std::max(0.0f, m_State.lootPickupTime - deltaTime);
    if (m_State.lootTime > 0)
    {
        m_State.moving = false;
        m_State.lootTime = std::max(0.0f, m_State.lootTime - deltaTime);
        if (m_State.lootTime == 0 && MonsterSystem::IsInteractableDrop(m_State.monster, m_State.player))
        {
            const int hunts = m_State.completedHunts;
            if (StorySystem::HasDrop(m_State))
            {
                if (const wchar_t *message = StorySystem::Interact(m_State)) ShowMessage(message);
            }
            else if (m_State.quest == QuestStage::Trophy)
            {
                const auto result = QuestSystem::TryInteract(m_State, m_State.world.elder, m_State.world.tracks, m_State.world.herb);
                if (const wchar_t *message = QuestSystem::GetInteractionMessage(result)) ShowMessage(message);
            }
            if (m_State.completedHunts > hunts) m_State.lootPickupTime = .6f;
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
    for (AnimalState &animal : m_State.animals)
    {
        animal.phase += deltaTime * animal.speed;
        animal.position.x = animal.home.x + std::sin(animal.phase) * 2.4f;
        animal.position.z = animal.home.z + std::cos(animal.phase * .7f) * 1.8f;
    }
    const float previousHealth = m_State.health;
    if ((m_State.quest == QuestStage::Hunt || StorySystem::IsHunting(m_State)) &&
        MonsterSystem::UpdateChaseAndAttack(m_State.monster, m_State.player, m_State.dodge, deltaTime, m_State.health, m_State.defense))
    {
        if (m_State.health < previousHealth)
        {
            m_State.playerHitTime = DamageFeedback::Duration;
            m_State.hitStop = DamageFeedback::HitStop;
            m_State.playerHitPosition = m_State.player;
            m_State.combo = 0;
            m_State.comboTime = 0;
        }
        ShowMessage(L"멧돼지에게 공격받았습니다. 공백 키로 회피하거나 Q로 회복하세요.");
    }
    if (m_State.health <= 0.0f)
    {
        m_State.player = m_State.world.spawn;
        m_State.health = m_State.maxHealth;
        ShowMessage(L"구조대의 도움으로 마을에 돌아왔습니다.");
    }
}

void GameManager::HandleCommand(GameCommand command)
{
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
            m_State.lootTime = 1.5f;
            m_State.moving = false;
            const Vec2 direction = {m_State.monster.position.x - m_State.player.x, m_State.monster.position.z - m_State.player.z};
            if (std::abs(direction.x) + std::abs(direction.z) > .001f) m_State.facing = direction;
            ShowMessage(L"전리품을 뒤지는 중...");
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
        m_State.attack = .32f;
        if (m_State.quest != QuestStage::Hunt && !StorySystem::IsHunting(m_State))
            return;
        const int healthBefore = m_State.monster.health;
        const MonsterHitResult result = MonsterSystem::Hit(m_State.monster, m_State.player, std::max(1, m_State.attackPower / 2));
        const int damage = healthBefore - m_State.monster.health;
        if (result.hit && damage > 0)
        {
            m_State.combo = m_State.comboTime > 0 ? m_State.combo + 1 : 1;
            m_State.comboTime = 2.0f;
            m_State.attackHitTime = DamageFeedback::Duration;
            m_State.hitStop = std::max(m_State.hitStop, m_State.combo % 3 == 0 ? .08f : .05f);
            if (m_State.damageNumbers.size() >= 16) m_State.damageNumbers.erase(m_State.damageNumbers.begin());
            m_State.damageNumbers.push_back({m_State.monster.position, damage, m_State.combo, .9f});
        }
        if (!result.defeated)
            return;
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
