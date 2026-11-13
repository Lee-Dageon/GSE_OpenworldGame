#include "stdafx.h"
#include "LootTable.h"
#include "BossIntro.h"
#include "StorySystem.h"
#include "QuestSystem.h"
#include "CraftingSystem.h"

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
    {
        state.monster = MonsterSystem::Create(GameContent::Get().monsters.at(step->monsterId),step->position);
        if (step->monsterId == "ancient_guardian") BossIntro::Start(state);
    }
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
bool SetProgressById(GameState &state, const std::string &chapterId, const std::string &stepId)
{
    const auto &steps = GameContent::Get().story.steps;
    for (size_t index = 0; index < steps.size(); ++index)
        if (steps[index].id == stepId && steps[index].chapterId == chapterId)
        {
            state.storyStep = static_cast<int>(index);
            state.storyFinished = false;
            return true;
        }
    return false;
}

bool DebugEnterStep(GameState &state, const std::string &stepId)
{
    const auto &content = GameContent::Get();
    const auto &steps = content.story.steps;
    size_t target = 0;
    while (target < steps.size() && steps[target].id != stepId) ++target;
    if (target == steps.size()) return false;

    GameState prepared = CreateInitialGameState();
    prepared.width = state.width;
    prepared.height = state.height;
    prepared.cameraZoom = state.cameraZoom;
    prepared.lastTime = state.lastTime;
    prepared.time = state.time;
    prepared.quest = QuestStage::Complete;
    prepared.hunterBladeCrafted = true;
    prepared.attackPower += 3;
    prepared.completedHunts = 1;
    QuestSystem::AddExperience(prepared, prepared.monster.definition.experience);
    // Reconstruct the debug prerequisites without replaying movement or dialogue.
    for (size_t index = 0; index < target; ++index)
    {
        const auto &previous = steps[index];
        if (previous.id == "ruin_control") prepared.ruinDeviceDisabled = true;
        if (previous.action == GameContent::StoryAction::Hunt)
            QuestSystem::AddExperience(prepared, content.monsters.at(previous.monsterId).experience);
        if (previous.action == GameContent::StoryAction::Collect)
        {
            if (index == 0) return false;
            if (!LootTable::Grant(prepared.inventory, LootTable::Monster(content.monsters.at(steps[index - 1].monsterId))))
                return false;
            ++prepared.completedHunts;
        }
        if (previous.action == GameContent::StoryAction::CraftArmor)
        {
            prepared.armorRecipeUnlocked = true;
            prepared.player = prepared.world.elder;
            CraftingSystem::TryCraft(prepared, CraftingSystem::Recipe::HunterArmor);
            if (!prepared.hunterArmorCrafted) return false;
        }
    }
    const auto &step = steps[target];
    if (!SetProgressById(prepared, step.chapterId, step.id)) return false;
    EnterStep(prepared);
    if (step.action == GameContent::StoryAction::Collect)
    {
        if (target == 0) return false;
        prepared.monster = MonsterSystem::Create(content.monsters.at(steps[target - 1].monsterId), step.position);
        prepared.monster.health = 0;
    }
    // Travel must remain pending on entry, rather than complete on the first update.
    prepared.player = step.action == GameContent::StoryAction::Travel ? prepared.world.spawn : step.position;
    prepared.message = L"디버그 목표 진입: " + step.objective;
    prepared.messageTime = 5.0f;
    state = prepared;
    return true;
}

bool DebugEnterChapter(GameState &state, const std::string &chapterId)
{
    for (const auto &step : GameContent::Get().story.steps)
        if (step.chapterId == chapterId) return DebugEnterStep(state, step.id);
    return false;
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
        if (!LootTable::Grant(state.inventory, LootTable::Monster(state.monster.definition)))
            return L"재료 보관 한도에 도달했습니다. 전리품은 남아 있습니다.";
        ++state.completedHunts;
        return Advance(state);
    }
    if ((step->action == GameContent::StoryAction::Investigate || step->action == GameContent::StoryAction::Report)
        && Near(state.player,step->position,step->radius))
    {
        if (step->id == "ruin_control") state.ruinDeviceDisabled = true;
        return Advance(state);
    }
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
