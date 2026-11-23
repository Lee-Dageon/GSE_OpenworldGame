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
#include "SaveGame.h"
#include "InventoryActions.h"
#include "LivingWorld.h"
#include "RegionSystem.h"
#include "PrologueSystem.h"
#include "WeaponMotion.h"

#include <algorithm>
#include <cmath>

GameManager::GameManager(GameState &state) : m_State(state) {}

void GameManager::BeginFrame(float deltaTime)
{
    m_State.time += deltaTime;
    m_State.endingTime = std::max(0.0f, m_State.endingTime-deltaTime);
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
        if (!WorldStreaming::Active(m_State,npc.position)) { npc.moving=false; continue; }
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
    if (m_State.prologueTime>=0)
    {
        m_State.prologueTime+=std::max(0.0f,deltaTime); m_State.time+=std::max(0.0f,deltaTime);
        if (m_State.prologueTime>=3) PrologueSystem::LoseEquipment(m_State);
        const float phase=std::clamp(m_State.prologueTime/8,0.0f,1.0f);
        m_State.player={m_State.world.spawn.x-18*(1-phase),m_State.world.spawn.z-15*(1-phase)};
        m_State.moving=true;
        if (phase>=1)
        {
            PrologueSystem::Finish(m_State);
        }
        return;
    }
    if (m_State.inventoryOpen || m_State.journalOpen)
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
    WorldStreaming::Update(m_State);
    m_State.herbRegrowTime = std::max(0.0f, m_State.herbRegrowTime - deltaTime);
    m_State.lootPickupTime = std::max(0.0f, m_State.lootPickupTime - deltaTime);
    if (m_State.lootTime > 0)
    {
        m_State.moving = false;
        m_State.lootTime = std::max(0.0f, m_State.lootTime - deltaTime);
        if (m_State.lootTime == 0 && m_State.lootNode >= 0)
        {
            const auto before=m_State.inventory.stacks;
            ShowMessage(HarvestSystem::Complete(m_State).c_str());
            int acquired=0;
            for (size_t i=0;i<before.size();++i) acquired+=std::max(0,m_State.inventory.stacks[i]-before[i]);
            LivingWorld::Record(m_State,1,acquired);
        }
        else if (m_State.lootTime == 0 && m_State.lootField >= 0)
        {
            const int index=m_State.lootField; m_State.lootField=-1;
            if (static_cast<size_t>(index)>=m_State.fieldMonsters.size()) return;
            auto &field=m_State.fieldMonsters[index];
            if (field.collected || !MonsterSystem::IsInteractableDrop(field.monster,m_State.player)) return;
            if (!LootTable::Grant(m_State.inventory,LootTable::Monster(field.monster.definition)))
            { ShowMessage(L"가방이 가득 찼습니다. 필드 전리품은 남아 있습니다."); return; }
            field.collected=true; field.respawnTime=120; ++m_State.completedHunts;
            AdventureEvents::Inspect(m_State,GameContent::Get().map.encounters[index].id,field.monster.definition.id);
            m_State.lootPickupTime=1; ShowMessage(L"필드 몬스터의 전리품을 수집했습니다.");
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
    WorldStreaming::Update(m_State);
    if (WeaponMotion::Release(m_State.weaponStyle,m_State.attack,m_State.attackPending)) ResolveAttack();
    UpdateNpcs(deltaTime);
    StorySystem::Update(m_State,deltaTime);
    BossIntro::TryStartVisible(m_State);
    if (BossIntro::Active(m_State)) { m_State.moving = false; return; }
    for (AnimalState &animal : m_State.animals)
    {
        if (!WorldStreaming::Active(m_State,animal.position) && animal.health>0 && !animal.collected) continue;
        WildlifeSystem::Update(animal,m_State,deltaTime);
    }
    LivingWorld::Update(m_State,deltaTime);
    AdventureEvents::Update(m_State,deltaTime);
    RegionSystem::Update(m_State,deltaTime);
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
        m_State.attack = 0; m_State.attackPending = false;
        ShowMessage(L"구조대의 도움으로 마을에 돌아왔습니다.");
    }
}

void GameManager::HandleCommand(GameCommand command)
{
    if (m_State.prologueTime>=0) return;
    if (BossIntro::Active(m_State)) return;
    if (command == GameCommand::ToggleInventory)
    {
        m_State.journalOpen=false;
        m_State.inventoryOpen = !m_State.inventoryOpen;
        m_State.moving = false;
        return;
    }
    if (m_State.hitStop > 0.0f) return;
    if (m_State.lootTime > 0.0f) return;
    if (command==GameCommand::Journal)
    {
        m_State.journalOpen=!m_State.journalOpen; m_State.inventoryOpen=false; m_State.moving=false; return;
    }
    if (command==GameCommand::BuildBase) { ShowMessage(LivingWorld::Build(m_State)); return; }
    if (m_State.journalOpen)
    {
        if (command==GameCommand::ChooseFirst) { LivingWorld::Accept(m_State); ShowMessage(L"다섯 의뢰를 동시에 수락했습니다."); }
        else if (command==GameCommand::ChooseSecond)
        {
            const auto *extra=AdventureEvents::Report(m_State);
            const auto *basic=LivingWorld::Report(m_State);
            ShowMessage(extra ? extra : basic);
        }
        else if (command==GameCommand::ChooseThird) ShowMessage(LivingWorld::HarvestFarm(m_State));
        else if (command==GameCommand::ChooseFourth) ShowMessage(AdventureEvents::Festival(m_State));
        else if (command==GameCommand::ChooseSixth) ShowMessage(AdventureEvents::StartTracking(m_State));
        else if (command==GameCommand::ChooseSeventh) ShowMessage(AdventureEvents::StartResearch(m_State));
        else if (command==GameCommand::ChooseEighth) ShowMessage(AdventureEvents::StartRaid(m_State));
        else if (command==GameCommand::ChooseNinth) ShowMessage(PrologueSystem::Recover(m_State));
        else if (command==GameCommand::Rest)
        {
            if (InventoryActions::AtStorage(m_State) && m_State.baseLevels[0]>0)
            {
                m_State.worldTime+=600; m_State.farmTime=std::min(1200.0f,m_State.farmTime+600);
                m_State.health=m_State.maxHealth; ShowMessage(L"거점에서 하루 쉬었습니다. 지난 축제는 4로 참여할 수 있습니다.");
                for (auto &npc:m_State.npcs) npc.health=100;
            }
            else ShowMessage(L"거점 집을 짓고 장로 옆에서 쉬세요.");
        }
        else if (command==GameCommand::SaveProgress || command==GameCommand::LoadProgress)
            ShowMessage(command==GameCommand::SaveProgress ? SaveGame::Write(m_State) : SaveGame::Read(m_State));
        return;
    }
    if (m_State.inventoryOpen)
    {
        switch (command)
        {
        case GameCommand::SelectPrevious: case GameCommand::SelectNext:
            m_State.inventorySelection=(m_State.inventorySelection+(command==GameCommand::SelectNext ? 1 : static_cast<int>(ItemId::Count)-1))%static_cast<int>(ItemId::Count);
            m_State.inventoryPage=m_State.inventorySelection/GameConstants::Inventory::ItemsPerPage;
            return;
        case GameCommand::SortInventory:
            m_State.inventorySort=(m_State.inventorySort+1)%3; return;
        case GameCommand::DiscardItem: ShowMessage(InventoryActions::Discard(m_State,m_State.sprint)); return;
        case GameCommand::DepositItem: ShowMessage(InventoryActions::Transfer(m_State,true,m_State.sprint)); return;
        case GameCommand::WithdrawItem: ShowMessage(InventoryActions::Transfer(m_State,false,m_State.sprint)); return;
        case GameCommand::EquipWeapon: ShowMessage(InventoryActions::CycleEquipment(m_State,true)); return;
        case GameCommand::EquipArmor: ShowMessage(InventoryActions::CycleEquipment(m_State,false)); return;
        case GameCommand::CraftStyles: ShowMessage(InventoryActions::CraftStyles(m_State)); return;
        default: break;
        }
    }
    if (command==GameCommand::SwitchStyle)
    {
        if (m_State.attack > 0 || m_State.attackPending) return;
        do { m_State.weaponStyle=(m_State.weaponStyle+1)%3; }
        while ((m_State.weaponStyle==1 && !m_State.spearCrafted) || (m_State.weaponStyle==2 && !m_State.bowCrafted));
        ShowMessage(m_State.weaponStyle==0 ? L"검 장착" : m_State.weaponStyle==1 ? L"창 장착 · 긴 사거리" : L"활 장착 · 원거리");
        return;
    }
    if (command == GameCommand::SaveProgress || command == GameCommand::LoadProgress)
    {
        ShowMessage(command == GameCommand::SaveProgress ? SaveGame::Write(m_State) : SaveGame::Read(m_State));
        return;
    }
    if (command == GameCommand::ChooseFirst || command == GameCommand::ChooseSecond)
    {
        if (const wchar_t *message = StorySystem::Choose(m_State, command == GameCommand::ChooseFirst ? 1 : 2))
            ShowMessage(message);
        return;
    }
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
        if (m_State.attackPending) return;
        if (const auto *message=AdventureEvents::Interact(m_State)) { ShowMessage(message); return; }
        if (const auto *message=RegionSystem::Interact(m_State)) { ShowMessage(message); return; }
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
        for (size_t i=0;i<m_State.fieldMonsters.size();++i)
        {
            const auto &field=m_State.fieldMonsters[i];
            if (!field.collected && MonsterSystem::IsInteractableDrop(field.monster,m_State.player))
            {
                m_State.lootField=static_cast<int>(i); m_State.lootNode=m_State.lootAnimal=-1;
                m_State.lootTime=GameConstants::Loot::Duration; m_State.moving=false;
                ShowMessage(L"필드 전리품을 수집합니다."); return;
            }
        }
        const auto board=GameContent::Get().map.noticeBoard;
        if (std::hypot(m_State.player.x-board.x,m_State.player.z-board.z)<1.2f &&
            std::hypot(m_State.player.x-m_State.world.elder.x,m_State.player.z-m_State.world.elder.z)>=1.5f)
        { m_State.journalOpen=true; ShowMessage(L"1 의뢰 수락 · 2 보고 · 3 수확 · 4 교역/축제 · 5 휴식"); return; }
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
    if (command == GameCommand::Attack && m_State.attack <= 0.0f && !m_State.attackPending)
    {
        m_State.attack = WeaponMotion::Duration(m_State.weaponStyle);
        m_State.attackPending = true;
        if (WeaponMotion::Release(m_State.weaponStyle,m_State.attack,m_State.attackPending)) ResolveAttack();
    }
}

void GameManager::ResolveAttack()
{
    const bool activeMonster = (m_State.quest == QuestStage::Hunt || StorySystem::IsHunting(m_State)) &&
        m_State.monster.health > 0;
    const float mx=m_State.monster.position.x-m_State.player.x, mz=m_State.monster.position.z-m_State.player.z;
    const float reach=m_State.weaponStyle==0 ? GameConstants::World::AnimalHitRange : InventoryActions::Range(m_State);
    if (m_State.weaponStyle==2)
    {
        const float length=std::sqrt(m_State.facing.x*m_State.facing.x+m_State.facing.z*m_State.facing.z);
        const float scale=reach/std::max(.001f,length);
        m_State.arrowTime=.3f; m_State.arrowFrom=m_State.player;
        m_State.arrowTo={m_State.player.x+m_State.facing.x*scale,m_State.player.z+m_State.facing.z*scale};
    }
    if (AdventureEvents::HitFestival(m_State,reach)) return;
    Vec2 extraPosition = {}; int extraDamage=0;
    if (RegionSystem::Attack(m_State,reach,std::max(1,m_State.attackPower/GameConstants::Player::DamageDivisor),extraPosition,extraDamage) ||
        AdventureEvents::AttackRaid(m_State,reach,std::max(1,m_State.attackPower/GameConstants::Player::DamageDivisor),extraPosition,extraDamage))
    {
        RecordHit(extraPosition,extraDamage);
        if (m_State.weaponStyle==2) { m_State.arrowTime=.3f; m_State.arrowFrom=m_State.player; m_State.arrowTo=extraPosition; }
        return;
    }
    float nearest = activeMonster ? std::min(reach*reach,mx*mx+mz*mz) : reach*reach;
    int animalIndex = -1;
    for (size_t i=0;i<m_State.animals.size();++i)
    {
        const auto &animal=m_State.animals[i];
        const float x=animal.position.x-m_State.player.x,z=animal.position.z-m_State.player.z;
        if (animal.health>0 && !animal.collected && x*x+z*z<nearest)
        { nearest=x*x+z*z; animalIndex=static_cast<int>(i); }
    }
    int fieldIndex=-1;
    for (size_t i=0;i<m_State.fieldMonsters.size();++i)
    {
        const auto &monster=m_State.fieldMonsters[i].monster;
        const float distance=std::hypot(monster.position.x-m_State.player.x,monster.position.z-m_State.player.z);
        if (monster.health>0 && distance*distance<nearest && WorldLayout::ClearLine(m_State.player,monster.position))
        { fieldIndex=static_cast<int>(i); nearest=distance*distance; }
    }
    if (fieldIndex>=0)
    {
        auto &field=m_State.fieldMonsters[fieldIndex];
        const int before=field.monster.health;
        const float range=field.monster.definition.hitRange;
        if (m_State.weaponStyle!=0) field.monster.definition.hitRange=reach;
        MonsterSystem::Hit(field.monster,m_State.player,std::max(1,m_State.attackPower/GameConstants::Player::DamageDivisor));
        field.monster.definition.hitRange=range;
        RecordHit(field.monster.position,before-field.monster.health);
        LivingWorld::RewardKill(m_State,field);
        if (m_State.weaponStyle==2) { m_State.arrowTime=.3f; m_State.arrowFrom=m_State.player; m_State.arrowTo=field.monster.position; }
        return;
    }
    if (animalIndex>=0)
    {
        auto &animal=m_State.animals[animalIndex];
        if (!WorldLayout::ClearLine(m_State.player,animal.position)) return;
        const int damage=WildlifeSystem::Hit(animal,m_State.player,
            std::max(GameConstants::Player::MinimumDamage,m_State.attackPower/GameConstants::Player::DamageDivisor));
        if (damage==0) return;
        RecordHit(animal.position,damage);
        if (m_State.weaponStyle==2) { m_State.arrowTime=.3f; m_State.arrowFrom=m_State.player; m_State.arrowTo=animal.position; }
        if (animal.health==0)
        { animal.deathTime = 0; QuestSystem::AddExperience(m_State,GameConstants::World::AnimalExperience); LivingWorld::Record(m_State,0); ShowMessage(L"동물을 사냥했습니다. E로 전리품을 수집하세요."); }
        return;
    }
    if (m_State.quest != QuestStage::Hunt && !StorySystem::IsHunting(m_State))
        return;
    if (!WorldLayout::ClearLine(m_State.player,m_State.monster.position)) return;
    const int healthBefore = m_State.monster.health;
    const float originalRange=m_State.monster.definition.hitRange;
    if (m_State.weaponStyle!=0) m_State.monster.definition.hitRange=InventoryActions::Range(m_State);
    const MonsterHitResult result = MonsterSystem::Hit(m_State.monster, m_State.player, std::max(GameConstants::Player::MinimumDamage, m_State.attackPower / GameConstants::Player::DamageDivisor));
    m_State.monster.definition.hitRange=originalRange;
    if (result.hit && m_State.weaponStyle==2) { m_State.arrowTime=.3f; m_State.arrowFrom=m_State.player; m_State.arrowTo=m_State.monster.position; }
    const int damage = healthBefore - m_State.monster.health;
    if (result.hit && damage > 0)
    {
        RecordHit(m_State.monster.position, damage);
    }
    if (!result.defeated)
        return;
    LivingWorld::Record(m_State,0);
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

const wchar_t *GameManager::GetHint() const
{
    for (const auto &entry:m_State.regions)
        if (m_State.player.x>=entry.second.minX && m_State.player.x<=entry.second.maxX &&
            m_State.player.z>=entry.second.minZ && m_State.player.z<=entry.second.maxZ)
            return L"E 고원 대화·조사·채집 · 목표는 주변 지도에 표시됩니다.";
    for (const auto &field:m_State.fieldMonsters)
        if (!field.collected && MonsterSystem::IsInteractableDrop(field.monster,m_State.player)) return L"E 필드 전리품 수집 · J 의뢰 일지";
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
