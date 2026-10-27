#include "stdafx.h"
#include "QuestSystem.h"

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
    switch (stage)
    {
    case QuestStage::Elder:
        return L"미라 장로와 대화하기";
    case QuestStage::Tracks:
        return L"멧돼지 흔적 조사하기";
    case QuestStage::Herb:
        return L"회복 약초 채집하기";
    case QuestStage::Hunt:
        return L"호숫가 멧돼지 사냥하기";
    case QuestStage::Trophy:
        return L"멧돼지 전리품 수집하기";
    case QuestStage::Return:
        return L"미라 장로에게 사냥 보고하기";
    case QuestStage::Complete:
        return L"엄니 검 제작 후 미라 장로 만나기";
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
        if (state.quest == QuestStage::Complete && state.hunterBladeCrafted)
        {
            state.monster = MonsterSystem::CreateLakeBoar();
            state.monster.maxHealth += std::min(8, state.completedHunts) * 6;
            state.monster.health = state.monster.maxHealth;
            state.quest = QuestStage::Hunt;
            return InteractionResult::RepeatHuntStarted;
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
        if (!state.inventory.Add(ItemId::Herb))
            return InteractionResult::InventoryFull;
        state.quest = QuestStage::Hunt;
        return InteractionResult::HerbCollected;
    }
    if (state.quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(state.monster, state.player))
    {
        if (state.inventory.Count(ItemId::BoarHide) >= Inventory::MaxStack ||
            state.inventory.Count(ItemId::BoarTusk) >= Inventory::MaxStack)
            return InteractionResult::InventoryFull;
        state.inventory.Add(ItemId::BoarHide);
        state.inventory.Add(ItemId::BoarTusk);
        ++state.completedHunts;
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
        return state.quest == QuestStage::Complete && state.hunterBladeCrafted
            ? L"E  다음 사냥 받기" : L"E  미라 장로와 대화 / I  가방";
    if (state.quest == QuestStage::Tracks && distance(state.player, state.world.tracks) < 1.3f)
        return L"E  흔적 조사";
    if (state.quest == QuestStage::Herb && distance(state.player, state.world.herb) < 1.3f)
        return L"E  약초 채집";
    if (state.quest == QuestStage::Trophy && MonsterSystem::IsInteractableDrop(state.monster, state.player))
        return L"E  전리품 수집";
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
        return L"미라 장로: 호숫가에 남은 흔적을 조사해 주세요.";
    case InteractionResult::QuestCompleted:
        return L"사냥 보고 완료! 장로 근처에서 I로 가방을 열고 C로 엄니 검을 제작하세요.";
    case InteractionResult::ElderDialogue:
        return L"미라 장로: 호숫가에서는 조심하세요.";
    case InteractionResult::TracksInvestigated:
        return L"멧돼지 흔적이 호수 쪽으로 이어집니다.";
    case InteractionResult::HerbCollected:
        return L"회복 약초를 채집했습니다.";
    case InteractionResult::TrophyCollected:
        return L"멧돼지 가죽 1개와 엄니 1개를 획득했습니다. 장로에게 돌아가세요.";
    case InteractionResult::NpcDialogue:
        return L"마을 주민: 호수 쪽에서 이상한 소리가 들려요.";
    case InteractionResult::InventoryFull:
        return L"재료 보관 한도(99개)에 도달했습니다. 전리품은 남아 있습니다.";
    case InteractionResult::RepeatHuntStarted:
        return L"다음 사냥을 받았습니다. 호숫가의 더 튼튼한 멧돼지를 사냥하세요.";
    case InteractionResult::None:
        return nullptr;
    }
    return nullptr;
}
} // namespace QuestSystem
