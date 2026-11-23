#include "../SimpleGame/SaveGame.h"
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/PrologueSystem.h"
#include <cassert>
#include <iostream>

static std::string Join(const SaveGame::LegacySections &s)
{
    std::ostringstream out;
    for (const auto &row:s.prefix) out<<row<<'\n';
    for (int i=0;i<3;++i) { out<<s.records[i].size()<<'\n'; for (const auto &row:s.records[i]) out<<row<<'\n'; }
    if (s.version==2)
    { for (const auto &row:s.world) out<<row<<'\n'; out<<s.records[3].size()<<'\n'; for (const auto &row:s.records[3]) out<<row<<'\n'; }
    out<<"end\n"; return out.str();
}

int main()
{
    GameState arrival=CreateInitialGameState(); const int base=arrival.attackPower;
    PrologueSystem::Start(arrival); assert(arrival.attackPower==base+7 && arrival.defense==6);
    GameManager(arrival).UpdateGame(3);
    assert(arrival.prologueLossApplied && arrival.attackPower==base && arrival.defense==0 && !arrival.hunterBladeCrafted);
    assert(arrival.lostWeaponTier==1 && arrival.lostArmorTier==1);
    PrologueSystem::Finish(arrival); const int lost=arrival.attackPower; PrologueSystem::Finish(arrival);
    assert(arrival.attackPower==lost);
    arrival.storyFinished=true; arrival.player=arrival.world.elder;
    PrologueSystem::Recover(arrival); assert(arrival.attackPower==base+7 && arrival.defense==6);
    PrologueSystem::Recover(arrival); assert(arrival.attackPower==base+7);
    arrival.lostWeaponTier=1; arrival.weaponTier=2; InventoryActions::Equip(arrival,true,2);
    const int improved=arrival.attackPower; PrologueSystem::Recover(arrival);
    assert(arrival.weaponTier==2 && arrival.attackPower==improved);
    GameState skip=CreateInitialGameState(); PrologueSystem::Start(skip);
    GameInput::OnKeyDown(skip,27,false); assert(skip.prologueSeen && skip.prologueLossApplied && skip.attackPower==2);

    GameState s=CreateInitialGameState(); s.player={980,600};
    RegionSystem::Stream(s); assert(s.regions.empty());
    s.storyFinished=true; s.endingChoice=1; s.quest=QuestStage::Complete; s.hunterBladeCrafted=true; s.equippedWeapon=0; s.attackPower+=3;
    RegionSystem::Stream(s); assert(s.regions.size()==1 && s.regionLoads==1);
    auto &region=s.regions.at("frost_reach"); assert(region.goals.size()==10 && region.enemies.size()==3 && region.gathers.size()==4);
    region.enemies[0].monster.health-=17; const int hurt=region.enemies[0].monster.health;
    region.gathers[0].timer=90; s.player=s.world.elder; RegionSystem::Stream(s);
    assert(s.regions.empty() && s.regionUnloads==1 && s.regionProgress.at("frost_reach").objects.at("frost_stalker").health==hurt);
    s.worldTime+=10; s.player={980,600}; RegionSystem::Stream(s);
    assert(s.regions.at("frost_reach").enemies[0].monster.health==hurt && s.regions.at("frost_reach").gathers[0].timer==80);
    assert(RegionSystem::Interact(s)); assert(s.regionProgress.at("frost_reach").started);
    s.inventory.Add(ItemId::FrostHerb,2); AdventureEvents::StartResearch(s);
    int goals=0;
    while (const auto *goal=RegionSystem::Goal(s.regions.at("frost_reach"),s.regionProgress.at("frost_reach")))
    {
        assert(++goals<=10);
        const auto before=goal->id; s.player=RegionSystem::Target(s.regions.at("frost_reach"),*goal);
        if (goal->action=="hunt")
        {
            Vec2 p={}; int damage=0;
            assert(RegionSystem::Attack(s,10,100000,p,damage) && damage>0);
            RegionSystem::Update(s,0);
        }
        else assert(RegionSystem::Interact(s));
        assert(s.regionProgress.at("frost_reach").stepId!=before);
    }
    assert(goals==10 && s.regionProgress.at("frost_reach").finished && s.variantSamples==2 && s.defense>=2);
    RegionSystem::Update(s,0); assert(s.weather==4);
    GameState restored=CreateInitialGameState();
    assert(SaveGame::Decode(SaveGame::Encode(s),s,restored));
    assert(restored.regions.empty() && restored.regionProgress.at("frost_reach").finished);
    RegionSystem::Stream(restored); assert(restored.regions.size()==1 && restored.regions.at("frost_reach").enemies[0].collected);
    const auto brokenDirectory=std::filesystem::current_path()/".."/"build"/"gameplay-checks"/("bad-region-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(brokenDirectory);
    { std::ofstream index(brokenDirectory/"index.gse"); index<<"gse_regions 1\nregion frost_reach frost_reach.gse 940 500 1380 860\n"; }
    { std::ofstream payload(brokenDirectory/"frost_reach.gse"); payload<<"gse_region 99 frost_reach \"broken\"\n"; }
    GameState rejected=CreateInitialGameState(); rejected.player={980,600}; rejected.endingChoice=1;
    bool rejectedFile=false;
    try { RegionSystem::Stream(rejected,brokenDirectory.string()); } catch (const std::runtime_error &) { rejectedFile=true; }
    assert(rejectedFile && rejected.regions.empty() && rejected.regionLoads==0);

    GameState tracking=CreateInitialGameState(); AdventureEvents::StartTracking(tracking);
    for (int i=0;i<3;++i) { tracking.player=AdventureEvents::TrackSites[i]; assert(AdventureEvents::Interact(tracking)); }
    tracking.player=tracking.world.elder; const int oldCoins=tracking.coins;
    assert(AdventureEvents::Report(tracking)); assert(tracking.coins==oldCoins+35 && !tracking.trackingActive);
    AdventureEvents::Report(tracking); assert(tracking.coins==oldCoins+35);
    AdventureEvents::StartResearch(tracking); AdventureEvents::Inspect(tracking,"one","storm_variant");
    AdventureEvents::Inspect(tracking,"one","storm_variant"); assert(tracking.variantSamples==1);
    AdventureEvents::Inspect(tracking,"two","forest_boar"); assert(tracking.variantSamples==1);
    AdventureEvents::Inspect(tracking,"two","mutated_guardian"); assert(tracking.variantSamples==2);
    AdventureEvents::Report(tracking); assert(!tracking.variantResearchActive && tracking.variantResearchCompleted==1);

    GameState festival=CreateInitialGameState(); festival.player=festival.world.elder; festival.worldTime=4200;
    for (int round=0;round<2;++round)
    {
        AdventureEvents::Festival(festival); assert(festival.festivalStage==1);
        const auto unchanged=festival.inventory.stacks;
        AdventureEvents::Festival(festival); assert(festival.inventory.stacks==unchanged && festival.festivalStage==1);
        festival.inventory.Add(ItemId::WildBerry,2); festival.inventory.Add(ItemId::PlantFiber,2);
        AdventureEvents::Festival(festival); assert(festival.festivalStage==2);
        for (const auto &point:AdventureEvents::LanternSites) { festival.player=point; assert(AdventureEvents::Interact(festival)); }
        assert(festival.festivalStage==3); festival.player=AdventureEvents::FestivalTarget();
        for (int i=0;i<3;++i) assert(AdventureEvents::HitFestival(festival,2));
        festival.player=festival.world.elder; const int before=festival.coins;
        AdventureEvents::Festival(festival); assert(festival.festivalClaimDay==round && festival.coins==before+25);
    }
    const int festivalCoins=festival.coins; AdventureEvents::Festival(festival); assert(festival.coins==festivalCoins);
    festival.sprint=true; AdventureEvents::Festival(festival); festival.sprint=false;
    const auto festivalItems=festival.inventory.stacks;
    AdventureEvents::Festival(festival); assert(festival.festivalStage==2 && festival.inventory.stacks==festivalItems);
    for (const auto &point:AdventureEvents::LanternSites) { festival.player=point; AdventureEvents::Interact(festival); }
    festival.player=AdventureEvents::FestivalTarget();
    for (int i=0;i<3;++i) AdventureEvents::HitFestival(festival,2);
    festival.player=festival.world.elder; AdventureEvents::Festival(festival);
    assert(festival.festivalStage==0 && festival.coins==festivalCoins && festival.inventory.stacks==festivalItems);

    GameState raid=CreateInitialGameState(); raid.player=raid.world.elder;
    AdventureEvents::StartRaid(raid); raid.worldEventTime=9.95f; AdventureEvents::Update(raid,.1f);
    assert(raid.worldEventStage==2 && raid.eventMonsters.size()==2);
    for (size_t i=0;i<2;++i)
    {
        raid.player=raid.eventMonsters[i].monster.position; Vec2 p={}; int damage=0;
        assert(AdventureEvents::AttackRaid(raid,2,100000,p,damage));
    }
    AdventureEvents::Update(raid,0); assert(raid.worldEventStage==3 && raid.worldEventKills==2);
    raid.player=raid.world.elder; AdventureEvents::Report(raid); assert(raid.coins==60 && raid.worldEventCompleted==1);
    AdventureEvents::StartRaid(raid); raid.worldEventTime=9.95f; AdventureEvents::Update(raid,.1f);
    raid.worldEventTime=119.95f; AdventureEvents::Update(raid,.1f);
    assert(raid.worldEventStage==3 && raid.worldEventDamage>0 && raid.npcs[1].health==75);

    // Added, removed and reordered map actors restore by ID, not vector position.
    GameState old=CreateInitialGameState(); old.npcs[0].health=42;
    SaveGame::LegacySections sections; assert(SaveGame::SplitLegacy(SaveGame::EncodeLegacy(old),sections));
    std::reverse(sections.records[2].begin(),sections.records[2].end());
    sections.records[2].erase(sections.records[2].begin()); // Added actor did not exist in this old save.
    sections.records[2].push_back("retired_scout 11 12 1 33");
    {
        std::istringstream items(sections.prefix[4]); size_t count=0; items>>count;
        std::ostringstream previous; previous<<30;
        for (int i=0,value=0;i<30;++i) { items>>value; previous<<' '<<value; }
        sections.prefix[4]=previous.str();
        std::istringstream stored(sections.world[1]); std::ostringstream previousStorage;
        for (int i=0,value=0;i<30;++i) { stored>>value; previousStorage<<(i ? " " : "")<<value; }
        sections.world[1]=previousStorage.str();
    }
    assert(SaveGame::Decode(Join(sections),old,restored));
    assert(restored.npcs[0].health==42 && restored.npcs.back().health==100 && restored.orphanRecords.count("npc/retired_scout"));
    auto covered=sections;
    {
        std::istringstream stats(covered.prefix[2]); float x=0,z=0; stats>>x>>z; std::string rest; std::getline(stats,rest);
        std::ostringstream placed; placed<<GameContent::Get().map.houses[0].position.x<<' '<<GameContent::Get().map.houses[0].position.z<<rest;
        covered.prefix[2]=placed.str();
    }
    GameState relocated; assert(SaveGame::Decode(Join(covered),old,relocated));
    assert(relocated.player.x==relocated.world.spawn.x && relocated.npcs[0].health==42);
    const auto preserved=SaveGame::Encode(restored);
    assert(SaveGame::Decode(preserved,restored,restored));
    const auto revived=SaveGame::RemapRecords("npc",{}, {"retired_scout 0 0 0 100"},restored.orphanRecords);
    assert(revived[0]=="retired_scout 11 12 1 33");
    auto corrupt=preserved.substr(0,preserved.size()/2); const int hp=static_cast<int>(restored.health);
    assert(!SaveGame::Decode(corrupt,restored,restored) && restored.health==hp);
    restored.worldTime=2000000; assert(SaveGame::Decode(SaveGame::Encode(restored),restored,restored));
    assert(restored.worldTime==2000000);
    const auto originalDirectory=std::filesystem::current_path();
    const auto migrationDirectory=originalDirectory/".."/"build"/"gameplay-checks"/("legacy-migration-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(migrationDirectory/"Saves"); std::filesystem::current_path(migrationDirectory);
    const auto previousFile=SaveGame::EncodeLegacy(old);
    { std::ofstream file("Saves/progress.gse",std::ios::binary); file<<previousFile; }
    SaveGame::Write(old); SaveGame::Write(old);
    { std::ifstream file("Saves/progress.pre-v3.gse",std::ios::binary); std::ostringstream contents; contents<<file.rdbuf(); assert(contents.str()==previousFile); }
    SaveGame::Read(restored); assert(restored.npcs[0].health==42);
    std::filesystem::current_path(originalDirectory);
    std::cout<<"PASS: equipment loss/recovery, actual regional file load/unload, 10 regional goals, tracking/research, interactive festivals, raid outcomes, ID-based save migration/archive\n";
}
