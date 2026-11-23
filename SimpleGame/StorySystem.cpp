#include "stdafx.h"
#include "LootTable.h"
#include "BossIntro.h"
#include "StorySystem.h"
#include "QuestSystem.h"
#include "CraftingSystem.h"
#include "WorldLayout.h"
#include "AdventureEvents.h"

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
        if (step->monsterId == "ancient_guardian") BossIntro::Arm(state);
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
        return StorySystem::Ending(state);
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
    prepared.equippedWeapon = 0;
    prepared.attackPower += 3;
    prepared.completedHunts = 1;
    QuestSystem::AddExperience(prepared, prepared.monster.definition.experience);
    // Reconstruct the debug prerequisites without replaying movement or dialogue.
    for (size_t index = 0; index < target; ++index)
    {
        const auto &previous = steps[index];
        if (previous.id == "ruin_control") prepared.ruinDeviceDisabled = true;
        if (previous.action == GameContent::StoryAction::Rescue)
        { ++prepared.rescuedSurvivors; prepared.escorts[previous.id=="rescue_scout" ? 1 : 0].arrived=true; }
        if (previous.action == GameContent::StoryAction::RouteChoice) prepared.migrationChoice = 1;
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
    if (step && step->action==GameContent::StoryAction::Rescue && state.escorts[step->id=="rescue_scout" ? 1 : 0].following)
        return {step->position.x+6,step->position.z};
    return step ? ((IsHunting(state) || HasDrop(state)) ? state.monster.position : step->position) : state.world.elder;
}
std::wstring Objective(const GameState &state)
{
    const auto *step = Current(state);
    if (!step) return state.storyFinished ? (state.endingChoice ? L"이야기 완료 · 장로에게 별빛 골짜기 변종 사냥 받기" :
        L"현재 이야기 완료 · 미라 장로에게 반복 사냥 받기") : L"미라 장로에게 본 이야기 의뢰 받기";
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
    if (step->action==GameContent::StoryAction::Rescue && state.escorts[step->id=="rescue_scout" ? 1 : 0].following)
        return L"동쪽 안전 표식까지 안내 · 멀어지면 돌아가 기다려 주세요.";
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
        AdventureEvents::Inspect(state,"story/"+step->id,state.monster.definition.id);
        return Advance(state);
    }
    if ((step->action == GameContent::StoryAction::Investigate || step->action == GameContent::StoryAction::Report ||
         step->action == GameContent::StoryAction::Rescue)
        && Near(state.player,step->position,step->radius))
    {
        if (step->id == "ruin_control") state.ruinDeviceDisabled = true;
        if (step->action == GameContent::StoryAction::Rescue)
        {
            auto &escort=state.escorts[step->id=="rescue_scout" ? 1 : 0];
            if (!escort.following) { escort.position=step->position; escort.following=true; }
            return L"구조 대상이 따라옵니다. 동쪽 6단위의 안전 표식까지 천천히 안내하세요. 멀어지면 기다립니다.";
        }
        return Advance(state);
    }
    return Near(state.player,state.world.elder,1.5f) ? step->hint.c_str() : nullptr;
}
const wchar_t *Ending(const GameState &state)
{
    if (state.endingChoice == 1)
        return state.migrationChoice == 2 ?
            L"균형의 계승: 이동로를 연 짐승들과 구조된 주민들이 함께 살아갑니다. 생태 장치를 복구했고 숨겨진 골짜기가 열렸습니다." :
            L"수호의 계승: 마을을 지키고 생태 장치를 복구했습니다. 사냥의 흔적을 기억하며 새로운 균형을 돌봅니다. 숨겨진 골짜기가 열렸습니다.";
    if (state.endingChoice == 2)
        return state.migrationChoice == 2 ?
            L"자연의 귀환: 짐승의 이동로를 열고 생태 장치를 멈췄습니다. 구조된 주민들과 스스로 회복하는 자연을 지켜봅니다. 숨겨진 골짜기가 열렸습니다." :
            L"자유의 대가: 마을을 지키고 생태 장치를 멈췄습니다. 주민들은 불안정한 기후에 대비하며 자연의 회복을 기다립니다. 숨겨진 골짜기가 열렸습니다.";
    return GameContent::Get().story.ending.c_str();
}
const wchar_t *Choose(GameState &state, int choice)
{
    const auto *step = Current(state);
    if (!step || state.inventoryOpen || (choice != 1 && choice != 2) ||
        !Near(state.player, step->position, step->radius)) return nullptr;
    if (step->action == GameContent::StoryAction::RouteChoice)
    {
        state.migrationChoice = choice;
        // Opening the route bypasses its hunt/collection pair and rejoins the investigation.
        if (choice == 2) state.storyStep += 2;
        Advance(state);
        return choice == 1 ? L"마을 방어를 선택했습니다. 길목의 위협을 사냥하세요. 주민 피해는 막지만 짐승의 희생이 남습니다." :
            L"이동로를 열었습니다. 주민과 짐승을 대피시켜 양쪽 피해를 줄였습니다. 생태 장치를 조사하세요.";
    }
    if (step->action != GameContent::StoryAction::FinalChoice) return nullptr;
    state.endingChoice = choice;
    state.endingTime = 14;
    return Advance(state);
}
const wchar_t *EnemyDefeated(GameState &state)
{
    return IsHunting(state) && !MonsterSystem::IsAlive(state.monster) ? Advance(state) : nullptr;
}
void Update(GameState &state,float deltaTime)
{
    const auto *step = Current(state);
    if (!step) return;
    if (step->action==GameContent::StoryAction::Rescue)
    {
        auto &escort=state.escorts[step->id=="rescue_scout" ? 1 : 0];
        if (!escort.following) return;
        const float distance=std::hypot(state.player.x-escort.position.x,state.player.z-escort.position.z);
        const int moves=std::max(1,static_cast<int>(std::ceil(std::max(0.0f,deltaTime)*3.2f/.12f)));
        for (int i=0;i<moves && distance<15 && distance>1;++i)
        {
            const float length=std::max(.001f,std::hypot(state.player.x-escort.position.x,state.player.z-escort.position.z));
            const float amount=std::min(length,std::max(0.0f,deltaTime)*3.2f/moves);
            const Vec2 next={escort.position.x+(state.player.x-escort.position.x)/length*amount,
                escort.position.z+(state.player.z-escort.position.z)/length*amount};
            if (WorldLayout::CanMove(state.world,escort.position,next,.12f)) escort.position=next;
            else
            {
                const Vec2 x={next.x,escort.position.z},z={escort.position.x,next.z};
                if (WorldLayout::CanMove(state.world,escort.position,x,.12f)) escort.position=x;
                else if (WorldLayout::CanMove(state.world,escort.position,z,.12f)) escort.position=z;
            }
        }
        const Vec2 safe={step->position.x+6,step->position.z};
        if (Near(state.player,safe,2) && Near(escort.position,safe,2))
        {
            escort.following=false; escort.arrived=true; ++state.rescuedSurvivors;
            if (state.contracts[3].active) state.contracts[3].progress=1;
            state.message=Advance(state); state.messageTime=7;
        }
        return;
    }
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
