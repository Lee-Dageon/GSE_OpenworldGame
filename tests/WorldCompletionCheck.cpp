#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/LivingWorld.h"
#include "../SimpleGame/SaveGame.h"
#include "../SimpleGame/PlayerController.h"
#include "../SimpleGame/NpcSystem.h"
#include <cassert>
#include <iostream>

int main()
{
    GameState s=CreateInitialGameState();
    s.inventoryOpen=true; s.player=s.world.elder;
    s.inventory.stacks.fill(0); s.inventory.Add(ItemId::Hardwood,20);
    s.inventorySelection=12;
    assert(InventoryActions::Selected(s)==ItemId::Hardwood);
    InventoryActions::Transfer(s,true,true);
    assert(s.inventory.Count(ItemId::Hardwood)==0 && s.storage.Count(ItemId::Hardwood)==20);
    InventoryActions::Transfer(s,false,false);
    assert(s.inventory.Count(ItemId::Hardwood)==1 && s.storage.Count(ItemId::Hardwood)==19);
    InventoryActions::Discard(s,false);
    assert(s.inventory.Count(ItemId::Hardwood)==0);
    s.inventory.Add(ItemId::Herb,8); s.inventorySort=1; s.inventorySelection=0;
    assert(InventoryActions::Selected(s)==ItemId::Herb);
    s.inventory.Add(ItemId::Hardwood,40); s.inventory.Add(ItemId::PlantFiber,6); s.inventory.Add(ItemId::BeastBone,4);
    InventoryActions::CraftStyles(s); InventoryActions::CraftStyles(s);
    assert(s.spearCrafted && s.bowCrafted && InventoryActions::Range(s)==10);
    s.hunterBladeCrafted=true; s.weaponTier=2;
    const int base=s.attackPower;
    InventoryActions::Equip(s,true,2); assert(s.attackPower==base+13);
    InventoryActions::Equip(s,true,0); assert(s.attackPower==base+3);
    InventoryActions::Equip(s,true,-1); assert(s.attackPower==base);
    s.inventory.Add(ItemId::CopperOre,30);
    for (int i=0;i<4;++i) LivingWorld::Build(s);
    for (int level:s.baseLevels) assert(level==1);
    LivingWorld::Accept(s);
    for (int i=0;i<5;++i) LivingWorld::Record(s,i,99);
    const int coins=s.coins;
    LivingWorld::Report(s); assert(s.coins>coins);
    const int paid=s.coins; LivingWorld::Report(s); assert(s.coins==paid);
    s.worldTime=4200; s.farmTime=120;
    assert(LivingWorld::Season(s)==1);
    const int herbs=s.inventory.Count(ItemId::Herb);
    LivingWorld::HarvestFarm(s); assert(s.inventory.Count(ItemId::Herb)==herbs+2);
    LivingWorld::TradeOrFestival(s); assert(s.festivalClaimDay==0);
    LivingWorld::TradeOrFestival(s); assert(s.festivalClaimDay==1);
    WorldStreaming::Update(s); assert(s.loadedChunks.size()==9);
    const int loads=s.chunkLoads;
    s.gatherNodes[0].regrowTime=50;
    s.player={500,500}; WorldStreaming::Update(s);
    assert(s.chunkUnloads>0 && s.chunkLoads>loads && !WorldStreaming::Active(s,s.world.elder));
    s.player=s.world.elder; WorldStreaming::Update(s);
    assert(s.gatherNodes[0].regrowTime==50);
    s.inventoryOpen=false;
    GameState restored;
    const auto encoded=SaveGame::Encode(s);
    assert(SaveGame::Decode(encoded,s,restored));
    assert(restored.storage.stacks==s.storage.stacks && restored.baseLevels==s.baseLevels);
    assert(restored.coins==s.coins && restored.fieldMonsters.size()==15 && restored.spearCrafted && restored.bowCrafted);
    auto invalid=encoded; invalid.replace(9,1,"9");
    assert(!SaveGame::Decode(invalid,s,restored));
    // V1 compatibility: remove the v2 extension and per-NPC health field.
    auto legacy=encoded.substr(0,encoded.find("world "));
    legacy.replace(9,1,"1");
    std::istringstream rows(legacy); std::string line,old;
    bool npc=false; size_t remaining=0;
    // NPC records are the final records before the extension, and have five tokens.
    while (std::getline(rows,line))
    {
        std::istringstream row(line); std::vector<std::string> fields; std::string token;
        while (row>>token) fields.push_back(token);
        if (fields.size()==5 && fields[0].rfind("npc_",0)==0) line=line.substr(0,line.find_last_of(' '));
        old+=line+'\n';
    }
    old+="end\n";
    assert(SaveGame::Decode(old,s,restored));
    const auto originalDirectory=std::filesystem::current_path();
    const auto sandbox=originalDirectory/".."/"build"/"gameplay-checks"/("save-fixture-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(sandbox); std::filesystem::current_path(sandbox);
    SaveGame::Write(s); assert(std::filesystem::exists("Saves/progress.gse"));
    ++s.coins; SaveGame::Write(s); ++s.coins; SaveGame::Write(s);
    assert(std::filesystem::exists("Saves/progress.bak"));
    GameState disk=CreateInitialGameState(); SaveGame::Read(disk);
    assert(disk.coins==s.coins && disk.storage.stacks==s.storage.stacks);
    const auto preserved=SaveGame::Encode(disk);
    { std::ofstream corrupt("Saves/progress.gse"); corrupt << "broken"; }
    SaveGame::Read(disk); assert(SaveGame::Encode(disk)==preserved);
    std::filesystem::current_path(originalDirectory);
    // Actual escort movement must reach the safe area before the story advances.
    GameState escort=CreateInitialGameState();
    assert(StorySystem::DebugEnterStep(escort,"rescue_traveler"));
    const int step=escort.storyStep;
    assert(StorySystem::Interact(escort)); assert(escort.storyStep==step && escort.escorts[0].following);
    escort.player=StorySystem::Target(escort);
    for (int i=0;i<100 && escort.storyStep==step;++i) StorySystem::Update(escort,.05f);
    assert(escort.storyStep==step+1 && escort.escorts[0].arrived && escort.rescuedSurvivors==1);
    auto field=s.fieldMonsters[0]; field.monster.health=0;
    const int xp=s.experience;
    LivingWorld::RewardKill(s,field); const int after=s.experience;
    LivingWorld::RewardKill(s,field); assert(s.experience==after && field.rewardGranted);
    GameState battle=CreateInitialGameState(); battle.fieldMonsters.resize(2); battle.player={0,-2.5f};
    for (auto &f:battle.fieldMonsters)
    { f.monster=MonsterSystem::Create(GameContent::Get().monsters.at("forest_boar"),battle.player); f.home=battle.player; }
    LivingWorld::Update(battle,0);
    assert(battle.health==100-2*GameContent::Get().monsters.at("forest_boar").damage);
    GameState ranger=CreateInitialGameState(); ranger.fieldMonsters.clear(); ranger.animals.clear();
    ranger.player={100,-100};
    ranger.quest=QuestStage::Hunt; ranger.bowCrafted=true; ranger.weaponStyle=2;
    ranger.monster=MonsterSystem::Create(GameContent::Get().monsters.at("forest_boar"),{109,-100});
    assert(WorldLayout::ClearLine(ranger.player,ranger.monster.position));
    const int targetHealth=ranger.monster.health;
    GameManager(ranger).HandleCommand(GameCommand::Attack);
    assert(ranger.monster.health<targetHealth && ranger.arrowTime>0);
    assert(!WorldLayout::ClearLine({-9,-1.7f},{-3,-1.7f}));
    auto &patient=s.npcs[1]; patient.health=30; s.inventory.Add(ItemId::Herb);
    NpcSystem::Interact(s,patient); assert(patient.health==80);
    // Swept movement cannot cross the house even with a one-second update.
    assert(!WorldLayout::CanMove(s.world,{-8,-1.7f},{-6,-1.7f}));
    GameState cinematic=CreateInitialGameState(); cinematic.prologueTime=0;
    GameManager(cinematic).UpdateGame(8);
    assert(cinematic.prologueTime<0 && cinematic.prologueSeen && cinematic.player.x==cinematic.world.spawn.x);
    std::cout << "PASS: inventory/storage/equipment, concurrent contracts, base/farm/festival, chunks, save v1/v2, escort, rewards, collision, prologue\n";
}
