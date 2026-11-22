// Run from SimpleGame when a C++ build/test is explicitly authorized.
#include "../SimpleGame/SaveGame.h"
#include "../SimpleGame/CraftingSystem.h"
#include "../SimpleGame/QuestSystem.h"
#include "../SimpleGame/PlayerController.h"
#include <cassert>
#include <iostream>

int main()
{
    for (int route : {1,2}) for (int ending : {1,2})
    {
        GameState s=CreateInitialGameState();
        assert(StorySystem::DebugEnterStep(s,"migration_decision"));
        const int before=s.storyStep;
        s.player=s.world.spawn;
        assert(!StorySystem::Choose(s,route) && s.storyStep==before);
        s.player=StorySystem::Current(s)->position;
        s.inventoryOpen=true;
        assert(!StorySystem::Choose(s,route));
        s.inventoryOpen=false;
        assert(StorySystem::Choose(s,route));
        assert(s.migrationChoice==route && s.rescuedSurvivors==2);
        assert(s.storyStep==before+(route==1 ? 1 : 3));
        // Walk the remainder by the same action handlers, bypassing combat duration only.
        while (const auto *step=StorySystem::Current(s))
        {
            s.player=StorySystem::Target(s);
            if (step->action==GameContent::StoryAction::Travel) StorySystem::Update(s);
            else if (step->action==GameContent::StoryAction::Hunt)
            {
                s.monster.health=0;
                assert(StorySystem::EnemyDefeated(s));
            }
            else if (step->action==GameContent::StoryAction::FinalChoice) assert(StorySystem::Choose(s,ending));
            else assert(StorySystem::Interact(s));
        }
        assert(s.storyFinished && s.endingChoice==ending && s.migrationChoice==route);
        assert(s.endingTime==14 && StorySystem::Ending(s));
        s.gatherNodes[0].regrowTime=42;
        s.animals[0].health=0; s.animals[0].collected=true; s.animals[0].respawnTime=23;
        GameState loaded=CreateInitialGameState();
        const auto bytes=SaveGame::Encode(s);
        assert(SaveGame::Decode(bytes,loaded,loaded));
        assert(loaded.storyFinished && loaded.endingChoice==ending && loaded.migrationChoice==route);
        assert(loaded.inventory.stacks==s.inventory.stacks && loaded.rescuedSurvivors==2);
        assert(loaded.gatherNodes[0].regrowTime==42 && loaded.animals[0].respawnTime==23);
        const auto preserved=SaveGame::Encode(loaded);
        assert(!SaveGame::Decode(bytes.substr(0,bytes.size()/2),loaded,loaded));
        assert(!SaveGame::Decode(bytes+"trailing",loaded,loaded));
        auto wrongVersion=bytes;
        wrongVersion.replace(9,1,"9");
        assert(!SaveGame::Decode(wrongVersion,loaded,loaded));
        assert(SaveGame::Encode(loaded)==preserved);
        loaded.player=loaded.world.elder;
        assert(QuestSystem::TryInteract(loaded,loaded.world.elder,loaded.world.tracks,loaded.world.herb)==
            QuestSystem::InteractionResult::RepeatHuntStarted);
        assert(loaded.monster.definition.id=="valley_variant");
        assert(SaveGame::Decode(SaveGame::Encode(loaded),loaded,loaded));
    }
    GameState gear=CreateInitialGameState();
    gear.player=gear.world.elder;
    gear.hunterBladeCrafted=gear.hunterArmorCrafted=gear.armorRecipeUnlocked=true;
    gear.equippedWeapon=gear.equippedArmor=0;
    gear.inventory.Add(ItemId::IronOre,6);
    const auto unchanged=gear.inventory.stacks;
    CraftingSystem::TryCraft(gear,CraftingSystem::Recipe::HunterBlade);
    assert(gear.inventory.stacks==unchanged && gear.weaponTier==0);
    gear.inventory.Add(ItemId::Coal,4);
    const int attack=gear.attackPower, defense=gear.defense;
    CraftingSystem::TryCraft(gear,CraftingSystem::Recipe::HunterBlade);
    CraftingSystem::TryCraft(gear,CraftingSystem::Recipe::HunterArmor);
    assert(gear.weaponTier==1 && gear.armorTier==1 && gear.attackPower==attack+4 && gear.defense==defense+3);
    gear.inventory.Add(ItemId::GuardianScale,8); gear.inventory.Add(ItemId::AncientCrystal,4);
    CraftingSystem::TryCraft(gear,CraftingSystem::Recipe::HunterBlade);
    CraftingSystem::TryCraft(gear,CraftingSystem::Recipe::HunterArmor);
    const auto full=gear.inventory.stacks;
    CraftingSystem::TryCraft(gear,CraftingSystem::Recipe::HunterBlade);
    assert(gear.inventory.stacks==full && gear.weaponTier==2 && gear.attackPower==attack+10 && gear.defense==defense+8);
    for (const char *id : {"mutated_guardian","final_guardian","valley_variant"})
    {
        auto monster=MonsterSystem::Create(GameContent::Get().monsters.at(id),{0,0});
        float health=100;
        assert(!MonsterSystem::UpdateChaseAndAttack(monster,{0,0},0,0,health));
        assert(health==100 && monster.chatPattern==0);
        assert(!MonsterSystem::UpdateChaseAndAttack(monster,{10,10},0,1.2f,health));
        assert(health==100 && monster.chatPattern==-1);
        monster.attackCooldown=0;
        MonsterSystem::UpdateChaseAndAttack(monster,{0,0},0,0,health);
        assert(!MonsterSystem::UpdateChaseAndAttack(monster,{0,0},1,1.2f,health));
        monster.attackCooldown=0;
        MonsterSystem::UpdateChaseAndAttack(monster,{0,0},0,0,health);
        assert(MonsterSystem::UpdateChaseAndAttack(monster,{0,0},0,1.2f,health));
        assert(health==100-monster.definition.damage);
    }
    GameState locked=CreateInitialGameState();
    locked.player={789.99f,700}; locked.keys['w']=true;
    PlayerController::UpdateMovement(locked,.05f);
    assert(locked.player.x<790);
    locked.endingChoice=1;
    PlayerController::UpdateMovement(locked,.05f);
    assert(locked.player.x>=790);
    std::cout << "PASS: four endings, route merge, save round trip/rejection, upgrades, telegraphs, valley gate\n";
}
