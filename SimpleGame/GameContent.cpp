#include "stdafx.h"
#include "GameContent.h"
#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace
{
class Reader
{
    std::ifstream input;
    std::string path;
    int lineNumber = 0;
  public:
    explicit Reader(const std::string &filename) : input(filename, std::ios::binary), path(filename)
    {
        if (!input) Fail("cannot open file");
        input.seekg(0, std::ios::end);
        const auto size = input.tellg();
        if (size < 0 || size > 262144) Fail("file size exceeds 256 KiB");
        input.seekg(0);
    }
    [[noreturn]] void Fail(const std::string &message) const
    {
        throw std::runtime_error(path+":"+std::to_string(lineNumber)+": "+message);
    }
    void Require(bool condition, const char *message) const { if (!condition) Fail(message); }
    bool Next(std::string &tag, std::istringstream &row)
    {
        std::string line;
        while (std::getline(input, line))
        {
            ++lineNumber;
            if (lineNumber == 1 && line.compare(0, 3, "\xef\xbb\xbf") == 0) line.erase(0, 3);
            Require(line.size() <= 4096, "line exceeds 4096 bytes");
            row.clear(); row.str(line); row.imbue(std::locale::classic());
            row >> std::ws;
            if (row.peek() == '#' || row.peek() == EOF) continue;
            row >> tag;
            return true;
        }
        Require(input.eof(), "file read failed");
        return false;
    }
    void End(std::istringstream &row) const
    {
        Require(!row.fail(), "missing or invalid field");
        row >> std::ws;
        Require(row.peek() == EOF, "unexpected trailing field");
    }
    std::string Id(std::istringstream &row) const
    {
        std::string value; row >> value;
        Require(!row.fail() && !value.empty() && value.size() <= 64, "invalid ID length");
        for (unsigned char c : value)
            Require(c < 128 && (std::isalnum(c) || c == '_' || c == '-' || c == '.'), "invalid ID character");
        return value;
    }
    std::wstring Text(std::istringstream &row) const
    {
        row >> std::ws;
        Require(row.peek() == '"', "text must be quoted");
        std::string utf8; row >> std::quoted(utf8);
        Require(!row.fail() && !utf8.empty() && utf8.size() <= 2048, "invalid text length");
        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        Require(length > 0 && length <= 512, "invalid UTF-8 or text exceeds 512 characters");
        std::wstring value(length, L'\0');
        Require(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), &value[0], length) == length,
                "UTF-8 conversion failed");
        for (wchar_t c : value) Require(c >= 32, "control character in text");
        return value;
    }
};

template <class... Values> void Read(Reader &reader, std::istringstream &row, Values &...values)
{
    const int unused[] = {0, ((void)(row >> values), 0)...};
    (void)unused;
    reader.Require(!row.fail(), "missing or invalid numeric field");
}
bool Range(float value, float low, float high) { return std::isfinite(value) && value >= low && value <= high; }
void Header(Reader &reader, std::istringstream &row, const char *expected)
{
    std::string tag; int version = 0;
    reader.Require(reader.Next(tag, row) && tag == expected, "missing format header");
    Read(reader, row, version);
    reader.Require(version == 1, "unsupported format version");
}
void Unique(Reader &reader, std::set<std::string> &ids, const std::string &id)
{
    reader.Require(ids.insert(id).second, "duplicate ID or singleton record");
}

GameContent::MapDefinition ReadMap(const std::string &path)
{
    Reader reader(path); std::istringstream row; Header(reader, row, "gse_map");
    GameContent::MapDefinition map; map.id = reader.Id(row); reader.End(row);
    std::set<std::string> ids, required;
    std::string tag;
    while (reader.Next(tag, row))
    {
        if (tag == "bounds")
        {
            Unique(reader, required, tag);
            Read(reader, row, map.world.minimumCoordinate, map.world.maximumCoordinate);
        }
        else if (tag == "spawn")
        {
            Unique(reader, required, tag); Read(reader, row, map.world.spawn.x, map.world.spawn.z);
        }
        else if (tag == "lake")
        {
            Unique(reader, required, tag); Read(reader, row, map.world.lake.x, map.world.lake.z, map.world.lakeRadius);
        }
        else if (tag == "notice_board")
        {
            Unique(reader, required, tag); Read(reader, row, map.noticeBoard.x, map.noticeBoard.z);
        }
        else if (tag == "quest_point")
        {
            const std::string role = reader.Id(row); Unique(reader, required, "point_"+role);
            Vec2 point; Read(reader, row, point.x, point.z);
            if (role == "elder") map.world.elder = point;
            else if (role == "tracks") map.world.tracks = point;
            else if (role == "herb") map.world.herb = point;
            else reader.Fail("unknown quest point role");
        }
        else if (tag == "encounter")
        {
            Unique(reader, required, tag); map.monsterId = reader.Id(row);
            Read(reader, row, map.monsterSpawn.x, map.monsterSpawn.z);
        }
        else
        {
            const std::string id = reader.Id(row); Unique(reader, ids, id);
            if (tag == "house")
            {
                GameContent::HouseDefinition item; item.id = id;
                Read(reader, row, item.position.x, item.position.z, item.roadBend.x, item.roadBend.z,
                     item.junction, item.halfWidth, item.halfDepth);
                reader.Require(Range(item.junction,-10000,10000) && Range(item.halfWidth,.1f,20) && Range(item.halfDepth,.1f,20), "invalid house geometry");
                map.houses.push_back(item);
                reader.Require(map.houses.size() <= GameContent::MaxRoadHouses, "too many houses (maximum 32)");
            }
            else if (tag == "tree")
            {
                GameContent::TreeDefinition item; item.id = id;
                Read(reader, row, item.position.x, item.position.z);
                const std::string kind = reader.Id(row);
                reader.Require(kind == "pine" || kind == "broadleaf", "unknown tree kind"); item.pine = kind == "pine";
                Read(reader, row, item.variant, item.radius);
                reader.Require(item.variant >= 0 && item.variant < 4 && Range(item.radius,.1f,10), "invalid tree variant or radius");
                map.trees.push_back(item); reader.Require(map.trees.size() <= 512, "too many trees");
            }
            else if (tag == "landmark")
            {
                GameContent::Scenery item; item.id = id; const std::string kind = reader.Id(row);
                const std::array<std::string,7> kinds = {{"boulder","bush","log","fence","cart","ruin","pine"}};
                const auto found = std::find(kinds.begin(), kinds.end(), kind);
                reader.Require(found != kinds.end(), "unknown landmark kind");
                item.kind = static_cast<GameContent::SceneryKind>(found-kinds.begin());
                Read(reader, row, item.position.x, item.position.z, item.height, item.radius);
                reader.Require(Range(item.height,1,512) && Range(item.radius,0,20), "invalid landmark size");
                map.landmarks.push_back(item); reader.Require(map.landmarks.size() <= 512, "too many landmarks");
            }
            else if (tag == "campfire")
            {
                GameContent::Placement item; item.id = id; Read(reader, row, item.position.x, item.position.z);
                map.campfires.push_back(item);
                reader.Require(map.campfires.size() <= GameContent::MaxCampfires, "too many campfires (maximum 32)");
            }
            else if (tag == "npc")
            {
                GameContent::NpcDefinition item; item.id = id; auto &npc = item.initial;
                Read(reader, row, npc.position.x, npc.position.z, npc.column, npc.row, npc.phase);
                reader.Require(npc.column >= 0 && npc.column < 4 && npc.row >= 0 && npc.row < 2 && Range(npc.phase,-1000,1000), "invalid NPC variant");
                map.npcs.push_back(item); reader.Require(map.npcs.size() <= 128, "too many NPCs");
            }
            else if (tag == "terrain")
            {
                TerrainStrip item; item.id=id;
                Read(reader,row,item.kind,item.start.x,item.start.z,item.end.x,item.end.z,item.halfWidth,item.height);
                reader.Require(item.kind>=0 && item.kind<=2 && Range(item.halfWidth,2,8) && Range(item.height,1,12) &&
                    Range(TerrainGeometry::Length(item),20,120),"invalid terrain strip");
                map.terrain.push_back(item);
                reader.Require(map.terrain.size()<=32,"too many terrain strips");
            }
            else if (tag == "gather")
            {
                GameContent::GatherDefinition item; item.id = id;
                Read(reader, row, item.initial.kind, item.initial.position.x, item.initial.position.z);
                reader.Require(item.initial.kind >= 0 && item.initial.kind < 16, "unknown gathering material");
                map.gatherNodes.push_back(item);
                reader.Require(map.gatherNodes.size() <= 2048, "too many gather nodes");
            }
            else if (tag == "animal")
            {
                GameContent::AnimalDefinition item; item.id = id; auto &animal = item.initial;
                Read(reader, row, animal.position.x, animal.position.z, animal.home.x, animal.home.z,
                     animal.column, animal.row, animal.phase, animal.speed);
                reader.Require(animal.column >= 0 && animal.column < 5 && animal.row == 2 && Range(animal.phase,-1000,1000) && Range(animal.speed,0,10), "invalid animal variant or motion");
                animal.health=GameConstants::World::AnimalHealths[animal.column];
                map.animals.push_back(item); reader.Require(map.animals.size() <= 128, "too many animals");
            }
            else reader.Fail("unknown map record: "+tag);
        }
        reader.End(row);
    }
    for (const char *name : {"bounds","spawn","lake","notice_board","encounter","point_elder","point_tracks","point_herb"})
        reader.Require(required.count(name) != 0, "missing required map record");
    reader.Require(Range(map.world.minimumCoordinate,-10000,10000) && Range(map.world.maximumCoordinate,-10000,10000)
        && map.world.maximumCoordinate-map.world.minimumCoordinate > 1 && map.world.maximumCoordinate-map.world.minimumCoordinate <= 10000,
        "invalid map bounds");
    const auto point = [&](Vec2 p) {
        reader.Require(Range(p.x,map.world.minimumCoordinate,map.world.maximumCoordinate) &&
            Range(p.z,map.world.minimumCoordinate,map.world.maximumCoordinate), "position outside map bounds or not finite");
    };
    point(map.world.spawn); point(map.world.elder); point(map.world.tracks); point(map.world.herb);
    point(map.world.lake); point(map.noticeBoard); point(map.monsterSpawn);
    reader.Require(Range(map.world.lakeRadius,.1f,1000), "invalid lake radius");
    for (const auto &item : map.houses) { point(item.position); point(item.roadBend); }
    for (const auto &item : map.trees) point(item.position);
    for (const auto &item : map.landmarks) point(item.position);
    for (const auto &item : map.campfires) point(item.position);
    for (const auto &item : map.npcs) point(item.initial.position);
    for (const auto &item : map.animals) { point(item.initial.position); point(item.initial.home); }
    for (const auto &item : map.terrain) { point(item.start); point(item.end); }
    for (const auto &item : map.gatherNodes)
    {
        const Vec2 p = item.initial.position;
        point(p);
        const float dx = p.x - map.world.lake.x, dz = p.z - map.world.lake.z;
        reader.Require(!GameContent::IsBlocked(map, p) && dx*dx + dz*dz > map.world.lakeRadius*map.world.lakeRadius,
            "gather node is blocked or underwater");
    }
    for (Vec2 p : {map.world.spawn, map.world.elder, map.world.tracks, map.world.herb, map.monsterSpawn})
    {
        const float x = p.x-map.world.lake.x, z = p.z-map.world.lake.z;
        reader.Require(x*x+z*z > map.world.lakeRadius*map.world.lakeRadius && !GameContent::IsBlocked(map,p), "spawn or quest point is blocked");
    }
    return map;
}

std::map<std::string, MonsterDefinition> ReadMonsters(const std::string &path)
{
    Reader reader(path); std::istringstream row; Header(reader,row,"gse_monsters"); reader.End(row);
    std::map<std::string, MonsterDefinition> result; std::set<std::string> appearances; std::string tag;
    while (reader.Next(tag,row))
    {
        if (tag == "appearance")
        {
            const std::string id = reader.Id(row); float height = 0; Read(reader,row,height); reader.End(row);
            reader.Require(result.count(id) != 0 && appearances.insert(id).second && Range(height,60,300), "invalid or duplicate monster appearance");
            result.at(id).spriteHeight = height;
            continue;
        }
        reader.Require(tag == "monster", "unknown monster record");
        MonsterDefinition item; item.id = reader.Id(row); item.name = reader.Text(row);
        Read(reader,row,item.health,item.hitRange,item.dropRange,item.chaseRange,item.attackRange,item.speed,
             item.damage,item.cooldown,item.experience,item.hideDrop,item.tuskDrop); reader.End(row);
        reader.Require(item.health >= 1 && item.health <= 10000 && item.experience >= 0 && item.experience <= 100000,
            "invalid monster health or experience");
        for (float value : {item.hitRange,item.dropRange,item.chaseRange,item.attackRange,item.speed,item.cooldown})
            reader.Require(Range(value,.01f,100), "invalid monster range, speed or cooldown");
        reader.Require(Range(item.damage,0,10000) && item.attackRange <= item.chaseRange, "invalid monster damage or chase range");
        reader.Require(item.hideDrop >= 1 && item.hideDrop <= Inventory::MaxStack && item.tuskDrop >= 1 && item.tuskDrop <= Inventory::MaxStack, "invalid loot amount");
        reader.Require(result.emplace(item.id,item).second, "duplicate monster ID");
        reader.Require(result.size() <= 128, "too many monster templates");
    }
    reader.Require(!result.empty(), "no monster templates"); return result;
}

GameContent::QuestDefinition ReadQuest(const std::string &path)
{
    Reader reader(path); std::istringstream row; Header(reader,row,"gse_quests");
    GameContent::QuestDefinition result; result.id = reader.Id(row); reader.End(row);
    const std::array<std::string,7> roles = {{"elder","tracks","herb","hunt","trophy","return","complete"}};
    std::array<bool,7> seen = {}; std::set<std::string> required;
    std::map<std::string,QuestStage> ids; std::string startId, tag;
    while (reader.Next(tag,row))
    {
        if (tag == "start")
        {
            Unique(reader,required,tag); startId = reader.Id(row); result.welcome = reader.Text(row);
        }
        else if (tag == "repeat")
        {
            Unique(reader,required,tag); Read(reader,row,result.repeatHealthStep,result.repeatHealthLimit);
            reader.Require(result.repeatHealthStep >= 1 && result.repeatHealthStep <= 1000 && result.repeatHealthLimit >= result.repeatHealthStep &&
                result.repeatHealthLimit <= 10000 && result.repeatHealthLimit % result.repeatHealthStep == 0, "invalid repeat health scaling");
            result.repeatObjective = reader.Text(row); result.repeatHint = reader.Text(row); result.repeatMessage = reader.Text(row);
        }
        else if (tag == "dialogue")
        {
            Unique(reader,required,tag); result.elderHint = reader.Text(row); result.elderDialogue = reader.Text(row); result.npcDialogue = reader.Text(row);
        }
        else if (tag == "stage")
        {
            const std::string id = reader.Id(row), role = reader.Id(row), next = reader.Id(row);
            const auto found = std::find(roles.begin(),roles.end(),role);
            reader.Require(found != roles.end(), "unsupported quest role");
            const size_t index = static_cast<size_t>(found-roles.begin());
            reader.Require(!seen[index], "duplicate quest role"); seen[index] = true;
            reader.Require(ids.emplace(id,static_cast<QuestStage>(index)).second, "duplicate quest stage ID");
            auto &step = result.steps[index]; step.id = id; step.nextId = next;
            step.objective = reader.Text(row); step.hint = reader.Text(row); step.message = reader.Text(row);
        }
        else reader.Fail("unknown quest record: "+tag);
        reader.End(row);
    }
    reader.Require(required.size() == 3 && std::all_of(seen.begin(),seen.end(),[](bool value){return value;}), "missing quest configuration or role");
    reader.Require(ids.count(startId) != 0, "unknown initial quest stage ID"); result.start = ids.at(startId);
    reader.Require(static_cast<int>(result.start) <= static_cast<int>(QuestStage::Hunt), "quest must start before loot collection");
    for (auto &step : result.steps)
    {
        reader.Require(ids.count(step.nextId) != 0, "unknown next quest stage ID"); step.next = ids.at(step.nextId);
    }
    reader.Require(result.steps[3].next == QuestStage::Trophy && result.steps[4].next == QuestStage::Return &&
        result.steps[5].next == QuestStage::Complete && result.steps[6].next == QuestStage::Hunt, "invalid hunt, loot, report or repeat transition");
    for (int first = 0; first < 3; ++first)
    {
        QuestStage cursor = static_cast<QuestStage>(first); std::set<int> visited;
        while (cursor != QuestStage::Hunt)
        {
            const int index = static_cast<int>(cursor);
            reader.Require(index < 3 && visited.insert(index).second, "quest cycle or transition skips the hunt");
            cursor = result.steps[index].next;
        }
    }
    return result;
}

GameContent::StoryDefinition ReadStory(const std::string &path, const GameContent::MapDefinition &map,
                                     const std::map<std::string,MonsterDefinition> &monsters)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return {};
        throw std::runtime_error(path+": cannot access optional story file");
    }
    Reader reader(path); std::istringstream row; Header(reader,row,"gse_story");
    GameContent::StoryDefinition story; story.id = reader.Id(row); reader.End(row);
    std::set<std::string> ids, required; std::map<std::string,int> chapters;
    const std::array<std::string,6> actions = {{"travel","investigate","hunt","collect","report","craft_armor"}};
    const auto point = [&](Vec2 p) {
        reader.Require(Range(p.x,map.world.minimumCoordinate,map.world.maximumCoordinate) &&
            Range(p.z,map.world.minimumCoordinate,map.world.maximumCoordinate) && !GameContent::IsBlocked(map,p), "blocked or invalid story target");
        const float x = p.x-map.world.lake.x, z = p.z-map.world.lake.z;
        reader.Require(x*x+z*z > map.world.lakeRadius*map.world.lakeRadius, "story target inside lake");
    };
    std::string tag; int lastChapter = -1;
    while (reader.Next(tag,row))
    {
        if (tag == "intro" || tag == "ending")
        {
            Unique(reader,required,tag);
            if (tag == "intro") story.introduction = reader.Text(row);
            else story.ending = reader.Text(row);
        }
        else if (tag == "chapter")
        {
            GameContent::StoryChapter chapter; chapter.id = reader.Id(row); Unique(reader,ids,chapter.id);
            chapter.title = reader.Text(row);
            chapters[chapter.id] = static_cast<int>(story.chapters.size());
            story.chapters.push_back(chapter); reader.Require(story.chapters.size() <= 16, "too many story chapters");
        }
        else if (tag == "region")
        {
            GameContent::StoryRegion region; region.id = reader.Id(row); Unique(reader,ids,region.id);
            region.name = reader.Text(row);
            Read(reader,row,region.minimumX,region.minimumZ,region.maximumX,region.maximumZ,
                 region.color[0],region.color[1],region.color[2]);
            reader.Require(Range(region.minimumX,map.world.minimumCoordinate,map.world.maximumCoordinate) &&
                Range(region.maximumX,map.world.minimumCoordinate,map.world.maximumCoordinate) &&
                Range(region.minimumZ,map.world.minimumCoordinate,map.world.maximumCoordinate) &&
                Range(region.maximumZ,map.world.minimumCoordinate,map.world.maximumCoordinate) &&
                region.minimumX < region.maximumX && region.minimumZ < region.maximumZ, "invalid story region bounds");
            for (float value : region.color) reader.Require(Range(value,0,1), "invalid story region color");
            story.regions.push_back(region); reader.Require(story.regions.size() <= GameContent::MaxStoryRegions, "too many story regions");
        }
        else if (tag == "step")
        {
            GameContent::StoryStep step; step.id = reader.Id(row); Unique(reader,ids,step.id);
            step.chapterId = reader.Id(row); const std::string action = reader.Id(row);
            reader.Require(chapters.count(step.chapterId) != 0 && chapters.at(step.chapterId) >= lastChapter, "unknown or backwards story chapter");
            lastChapter = chapters.at(step.chapterId);
            const auto found = std::find(actions.begin(),actions.end(),action);
            reader.Require(found != actions.end(), "unsupported story action");
            step.action = static_cast<GameContent::StoryAction>(found-actions.begin());
            Read(reader,row,step.position.x,step.position.z,step.radius); step.monsterId = reader.Id(row);
            reader.Require(Range(step.radius,.1f,10), "invalid story target radius"); point(step.position);
            if (step.action == GameContent::StoryAction::Hunt)
                reader.Require(monsters.count(step.monsterId) != 0, "unknown story monster ID");
            else reader.Require(step.monsterId == "none", "only hunt steps accept monster IDs");
            if (step.action == GameContent::StoryAction::CraftArmor)
            {
                const float x = step.position.x-map.world.elder.x, z = step.position.z-map.world.elder.z;
                reader.Require(x*x+z*z < 1.8f*1.8f, "armor goal must be at the workbench");
            }
            step.objective = reader.Text(row); step.hint = reader.Text(row); step.message = reader.Text(row);
            story.steps.push_back(step); reader.Require(story.steps.size() <= 128, "too many story steps");
        }
        else reader.Fail("unknown story record: "+tag);
        reader.End(row);
    }
    reader.Require(required.size() == 2 && !story.chapters.empty() && !story.steps.empty(), "incomplete story configuration");
    int hideBudget = 0, tuskBudget = 0; bool armorMade = false;
    for (size_t index = 0; index < story.steps.size(); ++index)
    {
        if (story.steps[index].action == GameContent::StoryAction::Hunt)
            reader.Require(index+1 < story.steps.size() && story.steps[index+1].action == GameContent::StoryAction::Collect &&
                story.steps[index+1].chapterId == story.steps[index].chapterId, "hunt must lead directly to collection in the same chapter");
        if (story.steps[index].action == GameContent::StoryAction::Collect)
        {
            reader.Require(index > 0 && story.steps[index-1].action == GameContent::StoryAction::Hunt, "orphan story collection step");
            const auto &loot = monsters.at(story.steps[index-1].monsterId);
            hideBudget += loot.hideDrop; tuskBudget += loot.tuskDrop;
        }
        if (story.steps[index].action == GameContent::StoryAction::CraftArmor && !armorMade)
        {
            reader.Require(hideBudget >= 2 && tuskBudget >= 1, "story cannot supply armor crafting materials");
            hideBudget -= 2; tuskBudget -= 1; armorMade = true;
        }
    }
    return story;
}
}

namespace GameContent
{
bool IsBlocked(const MapDefinition &map, Vec2 p)
{
    for (const auto &strip : map.terrain)
        if (TerrainGeometry::Blocked(strip,p)) return true;
    for (const auto &house : map.houses)
        if (std::abs(p.x-house.position.x) < house.halfWidth && std::abs(p.z-house.position.z) < house.halfDepth) return true;
    for (const auto &tree : map.trees)
        if ((p.x-tree.position.x)*(p.x-tree.position.x)+(p.z-tree.position.z)*(p.z-tree.position.z) < tree.radius*tree.radius) return true;
    for (const auto &item : map.landmarks)
        if (item.radius > 0 && (p.x-item.position.x)*(p.x-item.position.x)+(p.z-item.position.z)*(p.z-item.position.z) < (item.radius+.18f)*(item.radius+.18f)) return true;
    return false;
}
Content Load(const std::string &directory)
{
    Content result;
    result.map = ReadMap(directory+"/map.gse");
    result.monsters = ReadMonsters(directory+"/monsters.gse");
    result.quest = ReadQuest(directory+"/quests.gse");
    if (result.monsters.count(result.map.monsterId) == 0)
        throw std::runtime_error(directory+"/map.gse: encounter refers to unknown monster ID '"+result.map.monsterId+"'");
    result.story = ReadStory(directory+"/story.gse",result.map,result.monsters);
    return result;
}
const Content &Get()
{
    static const Content content = Load("./Data");
    return content;
}
const QuestStep &Step(QuestStage stage)
{
    const size_t index = static_cast<size_t>(stage);
    if (index >= Get().quest.steps.size()) throw std::runtime_error("invalid runtime quest stage");
    return Get().quest.steps[index];
}
}
