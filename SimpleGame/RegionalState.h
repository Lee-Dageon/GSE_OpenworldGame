#pragma once
#include "Monster.h"
#include <map>
#include <string>
#include <vector>

struct RegionObjectProgress
{
    Vec2 position = {};
    int health=-1;
    float timer=0;
    bool collected=false, rewarded=false;
};
struct RegionProgress
{
    std::string stepId;
    bool started=false, finished=false;
    int kills=0;
    double lastWorldTime=0;
    std::map<std::string,RegionObjectProgress> objects;
};
struct RegionSite
{
    std::string id;
    Vec2 position = {};
    int kind=0;
    std::wstring name, dialogue;
};
struct RegionEnemy
{
    std::string id;
    Vec2 home = {};
    MonsterState monster = {};
    bool collected=false,rewarded=false;
    float timer=0;
};
struct RegionGather
{
    std::string id;
    Vec2 position = {};
    int item=0,amount=1;
    float timer=0;
};
struct RegionGoal
{
    std::string id,action,target,next;
    std::wstring objective,message;
};
struct RegionRuntime
{
    std::string id;
    std::wstring name;
    float minX=0,minZ=0,maxX=0,maxZ=0;
    std::vector<RegionSite> sites;
    std::vector<RegionEnemy> enemies;
    std::vector<RegionGather> gathers;
    std::vector<RegionGoal> goals;
};
