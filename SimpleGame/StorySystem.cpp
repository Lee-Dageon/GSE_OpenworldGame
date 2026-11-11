#include "stdafx.h"
#include "StorySystem.h"

namespace
{
bool Near(Vec2 a, Vec2 b, float radius)
{
    const float x = a.x-b.x, z = a.z-b.z;
    return x*x+z*z < radius*radius;
}
void EnterStep(GameState &state)
{
    const auto *step = StorySystem::Current(state);
    if (!step) return;
    if (step->action == GameContent::StoryAction::Hunt)
        state.monster = MonsterSystem::Create(GameContent::Get().monsters.at(step->monsterId),step->position);
    if (step->action == GameContent::StoryAction::CraftArmor)
        state.armorRecipeUnlocked = true;
}
const wchar_t *Advance(GameState &state)
{
    const auto *previous = StorySystem::Current(state);
    if (!previous) return nullptr;
    ++state.storyStep;
    if (!StorySystem::Current(state))
    {
        state.storyFinished = true;
        return GameContent::Get().story.ending.c_str();
    }
    EnterStep(state);
    return previous->message.c_str();
}
}

namespace StorySystem
{
const GameContent::StoryStep *Current(const GameState &state)
{
    const auto &steps = GameContent::Get().story.steps;
    return !state.storyFinished && state.storyStep >= 0 && static_cast<size_t>(state.storyStep) < steps.size()
        ? &steps[state.storyStep] : nullptr;
}
bool CanStart(const GameState &state)
{
    return state.storyStep < 0 && !state.storyFinished && state.quest == QuestStage::Complete &&
        state.hunterBladeCrafted && !GameContent::Get().story.steps.empty();
}
bool IsHunting(const GameState &state)
{
    const auto *step = Current(state);
    return step && step->action == GameContent::StoryAction::Hunt;
}
bool HasDrop(const GameState &state)
{
    const auto *step = Current(state);
    return step && step->action == GameContent::StoryAction::Collect;
}
Vec2 Target(const GameState &state)
{
    const auto *step = Current(state);
    return step ? ((IsHunting(state) || HasDrop(state)) ? state.monster.position : step->position) : state.world.elder;
}
std::wstring Objective(const GameState &state)
{
    const auto *step = Current(state);
    if (!step) return state.storyFinished ? L"현재 이야기 완료 · 미라 장로에게 반복 사냥 받기" : L"미라 장로에게 본 이야기 의뢰 받기";
    const auto &chapters = GameContent::Get().story.chapters;
    for (const auto &chapter : chapters)
        if (chapter.id == step->chapterId) return chapter.title+L" · "+step->objective;
    return step->objective;
}
const wchar_t *Hint(const GameState &state)
{
    if (CanStart(state)) return Near(state.player,state.world.elder,1.5f) ? L"E  본 이야기 의뢰 받기" : nullptr;
    const auto *step = Current(state);
    if (!step) return nullptr;
    if (HasDrop(state)) return MonsterSystem::IsInteractableDrop(state.monster,state.player) ? step->hint.c_str() : nullptr;
    return Near(state.player,Target(state),step->radius) ? step->hint.c_str() : nullptr;
}
const wchar_t *RegionName(Vec2 position)
{
    const auto &regions = GameContent::Get().story.regions;
    if (regions.empty()) return L"그린웨이크";
    for (auto it = regions.rbegin(); it != regions.rend(); ++it)
        if (position.x >= it->minimumX && position.x <= it->maximumX && position.z >= it->minimumZ && position.z <= it->maximumZ)
            return it->name.c_str();
    return L"미개척지";
}
const wchar_t *Interact(GameState &state)
{
    if (CanStart(state))
    {
        if (!Near(state.player,state.world.elder,1.5f)) return nullptr;
        state.storyStep = 0; EnterStep(state);
        return GameContent::Get().story.introduction.c_str();
    }
    const auto *step = Current(state);
    if (!step) return nullptr;
    if (HasDrop(state) && MonsterSystem::IsInteractableDrop(state.monster,state.player))
    {
        const auto &loot = state.monster.definition;
        if (state.inventory.Count(ItemId::BoarHide) > Inventory::MaxStack-loot.hideDrop ||
            state.inventory.Count(ItemId::BoarTusk) > Inventory::MaxStack-loot.tuskDrop)
            return L"재료 보관 한도에 도달했습니다. 전리품은 남아 있습니다.";
        state.inventory.Add(ItemId::BoarHide,loot.hideDrop);
        state.inventory.Add(ItemId::BoarTusk,loot.tuskDrop);
        ++state.completedHunts;
        return Advance(state);
    }
    if ((step->action == GameContent::StoryAction::Investigate || step->action == GameContent::StoryAction::Report)
        && Near(state.player,step->position,step->radius))
        return Advance(state);
    return Near(state.player,state.world.elder,1.5f) ? step->hint.c_str() : nullptr;
}
const wchar_t *EnemyDefeated(GameState &state)
{
    return IsHunting(state) && !MonsterSystem::IsAlive(state.monster) ? Advance(state) : nullptr;
}
void Update(GameState &state)
{
    const auto *step = Current(state);
    if (!step) return;
    if ((step->action == GameContent::StoryAction::Travel && Near(state.player,step->position,step->radius)) ||
        (step->action == GameContent::StoryAction::CraftArmor && state.hunterArmorCrafted))
    {
        if (const wchar_t *message = Advance(state))
        {
            state.message = message;
            state.messageTime = 9;
        }
    }
}
}
