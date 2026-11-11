#include "stdafx.h"
#include "QuestSystem.h"
#include "GameContent.h"

#include <cmath>
#include <algorithm>

namespace QuestSystem
{
bool AddExperience(GameState &state, int amount)
{
    state.experience += amount;
    bool leveledUp = false;
    while (state.experience >= state.experienceToNextLevel)
    {
        state.experience -= state.experienceToNextLevel;
        ++state.level;
        ++state.attackPower;
        state.maxHealth += 15.0f;
        state.health = state.maxHealth;
        state.experienceToNextLevel += 10;
        leveledUp = true;
    }
    return leveledUp;
}

const wchar_t *GetObjective(QuestStage stage)
{
    return GameContent::Step(stage).objective.c_str();
}

InteractionResult TryInteract(GameState &state, Vec2 elder, Vec2 tracks, Vec2 herb)
{
    const auto distance = [](Vec2 a, Vec2 b)
    {
        const float x = a.x - b.x;
        const float z = a.z - b.z;
        return std::sqrt(x * x + z * z);
    };

    if (distance(state.player, elder) < 1.5f)
    {
        if (state.quest == QuestStage::Elder)
        {
            state.quest = GameContent::Step(state.quest).next;
            return InteractionResult::ElderQuestStarted;
        }
        if (state.quest == QuestStage::Return)
        {
            state.quest = GameContent::Step(state.quest).next;
            return InteractionResult::QuestCompleted;
        }
        if (state.quest == QuestStage::Complete && state.hunterBladeCrafted)
        {
            state.monster = MonsterSystem::CreateEncounter();
            const auto &quest = GameContent::Get().quest;
            state.monster.maxHealth += std::min(quest.repeatHealthLimit / quest.repeatHealthStep, state.completedHunts) * quest.repeatHealthStep;
            state.monster.health = state.monster.maxHealth;
            state.quest = GameContent::Step(state.quest).next;
            return InteractionResult::RepeatHuntStarted;
        }
        return InteractionResult::ElderDialogue;
    }
    if (state.quest == QuestStage::Tracks && distance(state.player, tracks) < 1.3f)
    {
        state.quest = GameContent::Step(state.quest).next;
        return InteractionResult::TracksInvestigated;
    }
    if (state.quest == QuestStage::Herb && distance(state.player, herb) < 1.3f)
    {
        if (!state.inventory.Add(ItemId::Herb))
            return InteractionResult::InventoryFull;
        state.quest = GameContent::Step(state.quest).next;
        return InteractionResult::HerbCollected;
    }
    if (state.quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(state.monster, state.player))
    {
        const auto &loot = state.monster.definition;
        if (state.inventory.Count(ItemId::BoarHide) > Inventory::MaxStack-loot.hideDrop ||
            state.inventory.Count(ItemId::BoarTusk) > Inventory::MaxStack-loot.tuskDrop)
            return InteractionResult::InventoryFull;
        state.inventory.Add(ItemId::BoarHide, loot.hideDrop);
        state.inventory.Add(ItemId::BoarTusk, loot.tuskDrop);
        ++state.completedHunts;
        state.quest = GameContent::Step(state.quest).next;
        return InteractionResult::TrophyCollected;
    }
    for (const NpcState &npc : state.npcs)
    {
        if (distance(state.player, npc.position) < 1.2f)
            return InteractionResult::NpcDialogue;
    }
    return InteractionResult::None;
}

const wchar_t *GetHint(const GameState &state)
{
    const auto distance = [](Vec2 a, Vec2 b)
    {
        const float x = a.x - b.x;
        const float z = a.z - b.z;
        return std::sqrt(x * x + z * z);
    };

    if (distance(state.player, state.world.elder) < 1.5f)
        return state.quest == QuestStage::Complete && state.hunterBladeCrafted
            ? GameContent::Get().quest.repeatHint.c_str() : GameContent::Get().quest.elderHint.c_str();
    if (state.quest == QuestStage::Tracks && distance(state.player, state.world.tracks) < 1.3f)
        return GameContent::Step(state.quest).hint.c_str();
    if (state.quest == QuestStage::Herb && distance(state.player, state.world.herb) < 1.3f)
        return GameContent::Step(state.quest).hint.c_str();
    if (state.quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(state.monster, state.player))
        return GameContent::Step(state.quest).hint.c_str();
    for (const NpcState &npc : state.npcs)
    {
        if (distance(state.player, npc.position) < 1.2f)
            return L"E  대화";
    }
    return nullptr;
}

const wchar_t *GetInteractionMessage(InteractionResult result)
{
    switch (result)
    {
    case InteractionResult::ElderQuestStarted:
        return GameContent::Step(QuestStage::Elder).message.c_str();
    case InteractionResult::QuestCompleted:
        return GameContent::Step(QuestStage::Return).message.c_str();
    case InteractionResult::ElderDialogue:
        return GameContent::Get().quest.elderDialogue.c_str();
    case InteractionResult::TracksInvestigated:
        return GameContent::Step(QuestStage::Tracks).message.c_str();
    case InteractionResult::HerbCollected:
        return GameContent::Step(QuestStage::Herb).message.c_str();
    case InteractionResult::TrophyCollected:
        return GameContent::Step(QuestStage::Trophy).message.c_str();
    case InteractionResult::NpcDialogue:
        return GameContent::Get().quest.npcDialogue.c_str();
    case InteractionResult::InventoryFull:
        return L"재료 보관 한도(99개)에 도달했습니다. 전리품은 남아 있습니다.";
    case InteractionResult::RepeatHuntStarted:
        return GameContent::Get().quest.repeatMessage.c_str();
    case InteractionResult::None:
        return nullptr;
    }
    return nullptr;
}
} // namespace QuestSystem
