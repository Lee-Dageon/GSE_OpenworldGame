#include "stdafx.h"
#include "QuestSystem.h"

#include <cmath>

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
    switch (stage)
    {
    case QuestStage::Elder:
        return L"Talk to Elder Mira";
    case QuestStage::Tracks:
        return L"Investigate the tracks";
    case QuestStage::Herb:
        return L"Gather the healing herb";
    case QuestStage::Hunt:
        return L"Hunt the lake boar";
    case QuestStage::Trophy:
        return L"Collect the boar trophy";
    case QuestStage::Return:
        return L"Return to Elder Mira";
    case QuestStage::Complete:
        return L"Quest complete";
    }
    return L"";
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
            state.quest = QuestStage::Tracks;
            return InteractionResult::ElderQuestStarted;
        }
        if (state.quest == QuestStage::Return)
        {
            state.quest = QuestStage::Complete;
            return InteractionResult::QuestCompleted;
        }
        return InteractionResult::ElderDialogue;
    }
    if (state.quest == QuestStage::Tracks && distance(state.player, tracks) < 1.3f)
    {
        state.quest = QuestStage::Herb;
        return InteractionResult::TracksInvestigated;
    }
    if (state.quest == QuestStage::Herb && distance(state.player, herb) < 1.3f)
    {
        ++state.herbs;
        state.quest = QuestStage::Hunt;
        return InteractionResult::HerbCollected;
    }
    if (state.quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(state.monster, state.player))
    {
        ++state.trophies;
        state.quest = QuestStage::Return;
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
        return L"E  Talk to Elder Mira";
    if (state.quest == QuestStage::Tracks && distance(state.player, state.world.tracks) < 1.3f)
        return L"E  Investigate tracks";
    if (state.quest == QuestStage::Herb && distance(state.player, state.world.herb) < 1.3f)
        return L"E  Gather herb";
    if (state.quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(state.monster, state.player))
        return L"E  Collect trophy";
    for (const NpcState &npc : state.npcs)
    {
        if (distance(state.player, npc.position) < 1.2f)
            return L"E  Talk";
    }
    return nullptr;
}

const wchar_t *GetInteractionMessage(InteractionResult result)
{
    switch (result)
    {
    case InteractionResult::ElderQuestStarted:
        return L"Elder Mira: Follow the tracks by the lake.";
    case InteractionResult::QuestCompleted:
        return L"Quest complete! You brought peace back to the village.";
    case InteractionResult::ElderDialogue:
        return L"Elder Mira: Please be careful near the lake.";
    case InteractionResult::TracksInvestigated:
        return L"The tracks lead toward the lake.";
    case InteractionResult::HerbCollected:
        return L"You collected a healing herb.";
    case InteractionResult::TrophyCollected:
        return L"You collected the boar trophy.";
    case InteractionResult::NpcDialogue:
        return L"Villager: Strange noises have been coming from the lake.";
    case InteractionResult::None:
        return nullptr;
    }
    return nullptr;
}
} // namespace QuestSystem
