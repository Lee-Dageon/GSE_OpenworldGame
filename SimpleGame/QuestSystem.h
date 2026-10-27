#pragma once

#include "GameState.h"

namespace QuestSystem
{
enum class InteractionResult
{
    None,
    ElderQuestStarted,
    QuestCompleted,
    ElderDialogue,
    TracksInvestigated,
    HerbCollected,
    TrophyCollected,
    NpcDialogue,
    InventoryFull,
    RepeatHuntStarted
};

bool AddExperience(GameState &state, int amount);
const wchar_t *GetObjective(QuestStage stage);
InteractionResult TryInteract(GameState &state, Vec2 elder, Vec2 tracks, Vec2 herb);
const wchar_t *GetHint(const GameState &state);
const wchar_t *GetInteractionMessage(InteractionResult result);
} // namespace QuestSystem
