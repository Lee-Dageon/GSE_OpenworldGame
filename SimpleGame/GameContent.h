#pragma once
#include "GameState.h"
#include <map>

namespace GameContent
{
static_assert(static_cast<int>(QuestStage::Complete) == 6, "Update content roles when adding quest stages");
constexpr int MaxRoadHouses = 32, MaxCampfires = 32;
constexpr int MaxStoryRegions = 16;
enum class SceneryKind { Boulder, Bush, Log, Fence, Cart, Ruin, Pine };
struct Placement { std::string id; Vec2 position = {}; };
struct HouseDefinition
{
    std::string id;
    Vec2 position = {}, roadBend = {};
    float junction = 0, halfWidth = 0, halfDepth = 0;
};
struct TreeDefinition
{
    std::string id;
    Vec2 position = {};
    bool pine = false;
    int variant = 0;
    float radius = 0;
};
struct Scenery
{
    std::string id;
    Vec2 position = {};
    SceneryKind kind = SceneryKind::Boulder;
    float height = 0, radius = 0;
};
struct NpcDefinition { std::string id; NpcState initial = {}; };
struct AnimalDefinition { std::string id; AnimalState initial = {}; };
struct MapDefinition
{
    std::string id, monsterId;
    WorldState world;
    Vec2 noticeBoard = {}, monsterSpawn = {};
    std::vector<HouseDefinition> houses;
    std::vector<TreeDefinition> trees;
    std::vector<Scenery> landmarks;
    std::vector<Placement> campfires;
    std::vector<NpcDefinition> npcs;
    std::vector<AnimalDefinition> animals;
};
struct QuestStep
{
    std::string id, nextId;
    QuestStage next = QuestStage::Complete;
    std::wstring objective, hint, message;
};
struct QuestDefinition
{
    std::string id;
    QuestStage start = QuestStage::Elder;
    std::array<QuestStep, 7> steps;
    int repeatHealthStep = 0, repeatHealthLimit = 0;
    std::wstring welcome, repeatObjective, repeatHint, repeatMessage;
    std::wstring elderHint, elderDialogue, npcDialogue;
};
enum class StoryAction { Travel, Investigate, Hunt, Collect, Report, CraftArmor };
struct StoryChapter { std::string id; std::wstring title; };
struct StoryRegion
{
    std::string id;
    std::wstring name;
    float minimumX = 0, minimumZ = 0, maximumX = 0, maximumZ = 0;
    std::array<float,3> color = {};
};
struct StoryStep
{
    std::string id, chapterId, monsterId;
    StoryAction action = StoryAction::Travel;
    Vec2 position = {};
    float radius = 0;
    std::wstring objective, hint, message;
};
struct StoryDefinition
{
    std::string id;
    std::wstring introduction, ending;
    std::vector<StoryChapter> chapters;
    std::vector<StoryRegion> regions;
    std::vector<StoryStep> steps;
};
struct Content
{
    MapDefinition map;
    std::map<std::string, MonsterDefinition> monsters;
    QuestDefinition quest;
    StoryDefinition story;
};

Content Load(const std::string &directory);
const Content &Get(); // Immutable, validated once; errors propagate to startup.
const QuestStep &Step(QuestStage stage);
bool IsBlocked(const MapDefinition &map, Vec2 position);
}
