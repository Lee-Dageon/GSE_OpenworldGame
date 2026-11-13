#pragma once
#include "GameState.h"
#include "WorldLayout.h"
#include <algorithm>
#include <cmath>

namespace WildlifeSystem
{
inline int Health(int species) { return GameConstants::World::AnimalHealths[std::max(0,std::min(4,species))]; }
inline int Hit(AnimalState &animal, Vec2 player, int power)
{
    if (animal.health<=0 || animal.collected || animal.hitProtection>0) return 0;
    const int damage=std::min(animal.health,std::max(1,power));
    animal.health-=damage;
    animal.hitFlash=GameConstants::Feedback::MonsterFlashDuration;
    animal.hitProtection=GameConstants::World::AnimalHitProtection;
    animal.scared=true;
    animal.fleeTime=GameConstants::World::AnimalFleeDuration;
    const float dx=animal.position.x-player.x, dz=animal.position.z-player.z, length=std::hypot(dx,dz);
    if (length>.001f) animal.facing={dx/length,dz/length};
    return damage;
}
inline void Update(AnimalState &animal, const GameState &state, float dt)
{
    animal.hitFlash=std::max(0.0f,animal.hitFlash-dt);
    animal.hitProtection=std::max(0.0f,animal.hitProtection-dt);
    if (animal.collected)
    {
        animal.respawnTime=std::max(0.0f,animal.respawnTime-dt);
        if (animal.respawnTime==0)
        {
            animal.health=Health(animal.column); animal.collected=false; animal.position=animal.home;
            animal.deathTime=animal.fleeTime=animal.hitProtection=0; animal.scared=false;
        }
        return;
    }
    if (animal.health<=0) { animal.deathTime+=dt; return; }
    animal.fleeTime=std::max(0.0f,animal.fleeTime-dt);
    const float dx=animal.position.x-state.player.x, dz=animal.position.z-state.player.z, distance=std::hypot(dx,dz);
    if (animal.scared && distance<10) animal.fleeTime=std::max(animal.fleeTime,1.0f);
    animal.phase+=dt*animal.speed;
    Vec2 direction=animal.facing;
    float speed=GameConstants::World::AnimalFleeSpeeds[animal.column];
    if (animal.fleeTime>0)
    {
        if (distance>.001f) direction={dx/distance,dz/distance};
    }
    else
    {
        const Vec2 target={animal.home.x+std::sin(animal.phase)*2.4f,animal.home.z+std::cos(animal.phase*.7f)*1.8f};
        const float tx=target.x-animal.position.x,tz=target.z-animal.position.z,length=std::hypot(tx,tz);
        if (length<.05f) return;
        direction={tx/length,tz/length};
        speed=std::min(.8f,std::max(.2f,animal.speed));
        speed=std::min(speed,length/std::max(.001f,dt));
    }
    const auto valid=[&](Vec2 p) {
        return p.x>state.world.minimumCoordinate && p.x<state.world.maximumCoordinate &&
            p.z>state.world.minimumCoordinate && p.z<state.world.maximumCoordinate &&
            std::hypot(p.x-state.world.lake.x,p.z-state.world.lake.z)>state.world.lakeRadius && !WorldLayout::IsBlocked(p);
    };
    // Small swept steps prevent a fast animal from jumping through a narrow blocker.
    const int steps=std::max(1,static_cast<int>(std::ceil(speed*dt/.2f)));
    for (int step=0;step<steps;++step)
    {
        bool moved=false;
        for (int attempt=0;attempt<5;++attempt)
        {
            const float angle=attempt==0 ? 0 : (attempt%2 ? 1.0f : -1.0f)*((attempt+1)/2)*.65f;
            const float c=std::cos(angle),s=std::sin(angle);
            const Vec2 facing={direction.x*c-direction.z*s,direction.x*s+direction.z*c};
            const Vec2 next={animal.position.x+facing.x*speed*dt/steps,animal.position.z+facing.z*speed*dt/steps};
            if (valid(next)) { animal.position=next; animal.facing=facing; moved=true; break; }
        }
        if (!moved) break;
    }
}
}
