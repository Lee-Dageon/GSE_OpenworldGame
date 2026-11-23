#include "../SimpleGame/Navigation.h"
#include "../SimpleGame/LivingWorld.h"
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/SaveGame.h"
#include <cassert>
#include <iostream>

int main()
{
    GameState s=CreateInitialGameState();
    const Vec2 start={-9,-1.7f},target={-3,-1.7f};
    NavigationRoute route;
    assert(Navigation::Plan(s.world,start,target,route) && route.points.size()>1);
    Vec2 previous=start;
    for (const auto point:route.points)
    { assert(WorldLayout::CanTravel(s.world,previous,point,.12f)); previous=point; }
    assert(Navigation::Distance(previous,target)<.01f);
    auto position=start;
    assert(!Navigation::Move(s.world,position,target,1,0,route));
    assert(position.x==start.x && position.z==start.z);
    assert(!Navigation::Plan(s.world,start,{-6,-1.7f},route) && route.points.empty());
    assert(!Navigation::Plan(s.world,start,{100,-1.7f},route)); // Outside the bounded search.
    assert(!Navigation::Move(s.world,position,route.target,6,.1f,route));
    assert(route.retry>0 && route.points.empty());
    const Vec2 changed={start.x,start.z-3};
    assert(Navigation::Move(s.world,position,changed,.2f,.05f,route));
    assert(Navigation::Distance(route.target,changed)<.01f); // New targets clear a failed plan.

    s.fieldMonsters.clear(); s.animals.clear(); s.npcs.clear();
    s.player=target; s.companion=start; s.baseLevels[3]=1;
    bool usedRoute=false;
    for (int i=0;i<160;++i)
    {
        previous=s.companion; LivingWorld::Update(s,.05f);
        assert(WorldLayout::CanTravel(s.world,previous,s.companion,.12f));
        assert(Navigation::Distance(previous,s.companion)<=.191f);
        usedRoute=usedRoute || !s.companionRoute.points.empty();
    }
    assert(usedRoute && Navigation::Distance(s.companion,target)<1.6f);
    s.companion=start; s.companionRoute={};
    GameManager manager(s); s.inventoryOpen=true; manager.UpdateGame(.5f);
    assert(s.companion.x==start.x && s.companion.z==start.z);
    s.inventoryOpen=false; s.journalOpen=true; manager.UpdateGame(.5f);
    assert(s.companion.x==start.x && s.companion.z==start.z);

    GameState escort=CreateInitialGameState();
    assert(StorySystem::DebugEnterStep(escort,"rescue_traveler"));
    assert(StorySystem::Interact(escort));
    escort.player=target; escort.escorts[0].position=start;
    for (int i=0;i<160;++i)
    {
        previous=escort.escorts[0].position; StorySystem::Update(escort,.05f);
        assert(WorldLayout::CanTravel(escort.world,previous,escort.escorts[0].position,.12f));
        assert(Navigation::Distance(previous,escort.escorts[0].position)<=.161f);
    }
    assert(Navigation::Distance(escort.escorts[0].position,target)<1.1f && escort.escorts[0].following);
    escort.player={30,30}; previous=escort.escorts[0].position;
    StorySystem::Update(escort,1);
    assert(Navigation::Distance(previous,escort.escorts[0].position)==0); // Too far: wait.

    GameState saved=CreateInitialGameState(),loaded;
    assert(Navigation::Plan(saved.world,start,target,saved.companionRoute));
    saved.escorts[0].route=saved.companionRoute;
    assert(SaveGame::Decode(SaveGame::Encode(saved),saved,loaded));
    assert(loaded.companionRoute.points.empty() && loaded.escorts[0].route.points.empty());
    assert(Navigation::Plan(loaded.world,start,target,loaded.companionRoute));
    std::cout << "PASS: bounded routes, collision-safe companion/escort detours, retarget/retry, pause/wait and transient save state\n";
}
