// Standalone loader check, run from SimpleGame. Link GameContent.cpp and kernel32.lib; use C++17.
// Not run automatically: this project requires an explicit request before building.
#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/GameContent.h"
#include <Windows.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace fs = std::filesystem;
std::string ReadFile(const fs::path &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("fixture read failed");
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
void WriteFile(const fs::path &path, const std::string &value)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << value;
    if (!output) throw std::runtime_error("fixture write failed");
}
std::string Replace(std::string value, const std::string &before, const std::string &after)
{
    const auto offset = value.find(before);
    if (offset == std::string::npos) throw std::runtime_error("fixture replacement not found");
    value.replace(offset,before.size(),after);
    return value;
}
struct Fixture
{
    fs::path root = fs::absolute("GameContentCheck_"+std::to_string(GetCurrentProcessId()));
    std::map<std::string,std::string> originals;
    Fixture()
    {
        if (root.parent_path() != fs::current_path() || !fs::create_directory(root))
            throw std::runtime_error("refusing to overwrite existing fixture directory");
        for (const char *name : {"map.gse","monsters.gse","quests.gse","story.gse"})
            originals[name] = ReadFile(fs::path("Data")/name);
        Reset();
    }
    void Reset() const { for (const auto &file : originals) WriteFile(root/file.first,file.second); }
    void Reject() const
    {
        bool rejected = false;
        try { (void)GameContent::Load(root.string()); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    ~Fixture()
    {
        std::error_code ignored;
        for (const char *name : {"map.gse","monsters.gse","quests.gse","story.gse"}) fs::remove(root/name,ignored);
        fs::remove(root,ignored); // Remove only our empty directory, never recursively.
    }
};
int main()
{
    const auto &live = GameContent::Get();
    assert(live.map.id == "greenwake" && live.map.houses.size() == 8 && live.map.trees.size() == 40);
    assert(live.map.landmarks.size() == 42 && live.map.campfires.size() == 10);
    assert(live.map.npcs.size() == 20 && live.map.animals.size() == 8);
    assert(live.monsters.at("lake_boar").name == L"호숫가 멧돼지");
    assert(!GameContent::IsBlocked(live.map,live.map.world.spawn));
    assert(GameContent::IsBlocked(live.map,live.map.houses[0].position));
    assert(GameContent::Step(QuestStage::Hunt).next == QuestStage::Trophy);
    assert(live.story.chapters.size() == 3 && live.story.steps.size() == 22);
    assert(live.monsters.at("rampaging_boar").spriteHeight == 170);
    Fixture fixture;
    const auto &map = fixture.originals.at("map.gse");
    const auto &monsters = fixture.originals.at("monsters.gse");
    const auto &quests = fixture.originals.at("quests.gse");
    WriteFile(fixture.root/"map.gse",map+"house extra_cottage -40 -40 -35 -38 -20 1.55 1.4\n");
    assert(GameContent::Load(fixture.root.string()).map.houses.size() == 9);
    WriteFile(fixture.root/"map.gse",Replace(map,"encounter lake_boar","encounter forest_boar"));
    const auto alternate = GameContent::Load(fixture.root.string());
    assert(alternate.monsters.at(alternate.map.monsterId).health == 18);
    assert(live.map.monsterId == "lake_boar");
    WriteFile(fixture.root/"map.gse",Replace(map,"gse_map 1","gse_map 2")); fixture.Reject();
    WriteFile(fixture.root/"map.gse",map+"tree tree_00 -20 -20 pine 0 .42\n"); fixture.Reject();
    WriteFile(fixture.root/"map.gse",Replace(map,"spawn 0 -2.5","spawn 100000 0")); fixture.Reject();
    WriteFile(fixture.root/"map.gse",Replace(map,"spawn 0 -2.5","spawn -6 -1.7")); fixture.Reject();
    WriteFile(fixture.root/"map.gse",Replace(map,"lake 45 39 12","lake 45 39 nan")); fixture.Reject();
    WriteFile(fixture.root/"map.gse",Replace(map,"encounter lake_boar","encounter unknown")); fixture.Reject();
    WriteFile(fixture.root/"map.gse",map+"spawn 0 -2.5\n"); fixture.Reject();
    std::string excess = map;
    for (int i = 0; i < 25; ++i)
        excess += "house extra_"+std::to_string(i)+" -40 -40 -35 -38 -20 1.55 1.4\n";
    WriteFile(fixture.root/"map.gse",excess); fixture.Reject(); fixture.Reset();
    WriteFile(fixture.root/"monsters.gse",Replace(monsters," 12 1.9"," -1 1.9")); fixture.Reject();
    WriteFile(fixture.root/"monsters.gse",Replace(monsters,"호숫가 멧돼지",std::string(1,static_cast<char>(0xff)))); fixture.Reject(); fixture.Reset();
    WriteFile(fixture.root/"quests.gse",Replace(quests,"stage elder elder tracks","stage elder elder missing")); fixture.Reject();
    WriteFile(fixture.root/"quests.gse",Replace(quests,"stage tracks tracks herb","stage tracks tracks tracks")); fixture.Reject();
    WriteFile(fixture.root/"quests.gse",Replace(quests,"repeat 6 48","repeat 0 48")); fixture.Reject(); fixture.Reset();
    const auto &story = fixture.originals.at("story.gse");
    for (const auto &replacement : std::map<std::string,std::string>{
        {"gse_story 1","gse_story 2"},
        {"unfamiliar travel 8 -7","unfamiliar unknown 8 -7"},
        {"travel 8 -7","travel -6 -1.7"},
        {"1.5 rampaging_boar","1.5 missing_boar"},
        {"unfamiliar collect","unfamiliar report"},
        {"unfamiliar hunt","missing_chapter hunt"}})
    {
        WriteFile(fixture.root/"story.gse",Replace(story,replacement.first,replacement.second));
        fixture.Reject(); fixture.Reset();
    }
    WriteFile(fixture.root/"monsters.gse",Replace(monsters,"35 3 2","35 1 1"));
    fixture.Reject(); fixture.Reset(); // Reject an armor objective without enough story rewards.
    WriteFile(fixture.root/"map.gse",map+std::string(262144,' ')); fixture.Reject(); fixture.Reset();
    fs::remove(fixture.root/"story.gse");
    assert(GameContent::Load(fixture.root.string()).story.steps.empty());
    fixture.Reset();
    fs::remove(fixture.root/"map.gse"); fixture.Reject();
    assert(live.map.houses.size() == 8 && live.map.monsterId == "lake_boar");
    std::cout << "PASS: default data, extra house, alternate monster, invalid versions/IDs/references/ranges/UTF-8/quest cycles/limits, missing file and immutable live content\n";
}
