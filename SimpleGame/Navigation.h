#pragma once
#include "WorldLayout.h"
#include <array>
#include <queue>
#include <limits>
#include <algorithm>
#include <functional>

namespace Navigation
{
inline float Distance(Vec2 a,Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
inline bool Plan(const WorldState &world,Vec2 start,Vec2 target,NavigationRoute &route)
{
    route.points.clear(); route.next=0; route.target=target; route.retry=.5f;
    if (!WorldLayout::CanMove(world,start,start,.12f) || !WorldLayout::CanMove(world,target,target,.12f)) return false;
    if (WorldLayout::CanTravel(world,start,target,.12f)) { route.points.push_back(target); return true; }
    // Local, bounded search; no world-sized allocation or unbounded retry each frame.
    constexpr int radius=32,side=radius*2+1,count=side*side;
    constexpr float cell=.75f;
    if (std::abs(target.x-start.x)>radius*cell || std::abs(target.z-start.z)>radius*cell) return false;
    const auto point=[&](int id) { return Vec2{start.x+(id%side-radius)*cell,start.z+(id/side-radius)*cell}; };
    std::array<float,count> cost; cost.fill(std::numeric_limits<float>::max());
    std::array<int,count> parent; parent.fill(-1);
    std::array<bool,count> closed={};
    using Entry=std::pair<float,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> open;
    const int origin=radius*side+radius;
    cost[origin]=0; open.push({Distance(start,target),origin});
    int expanded=0;
    while (!open.empty() && expanded<1024)
    {
        const int id=open.top().second; open.pop();
        if (closed[id]) continue;
        closed[id]=true; ++expanded;
        const Vec2 here=point(id);
        if (Distance(here,target)<=cell*1.5f && WorldLayout::CanTravel(world,here,target,.12f))
        {
            route.points.push_back(target);
            for (int current=id;current!=origin;current=parent[current]) route.points.push_back(point(current));
            std::reverse(route.points.begin(),route.points.end());
            return true;
        }
        for (int z=-1;z<=1;++z) for (int x=-1;x<=1;++x)
        {
            if (!x && !z) continue;
            const int nx=id%side+x,nz=id/side+z;
            if (nx<0 || nz<0 || nx>=side || nz>=side) continue;
            const int next=nz*side+nx;
            if (closed[next]) continue;
            const Vec2 destination=point(next);
            const float candidate=cost[id]+Distance(here,destination);
            if (candidate>=cost[next] || !WorldLayout::CanTravel(world,here,destination,.12f)) continue;
            cost[next]=candidate; parent[next]=id;
            open.push({candidate+Distance(destination,target),next});
        }
    }
    return false;
}
inline bool Move(const WorldState &world,Vec2 &position,Vec2 target,float distance,float dt,NavigationRoute &route)
{
    if (distance<=0 || dt<=0) return false;
    route.retry=std::max(0.0f,route.retry-dt);
    if (Distance(route.target,target)>1.5f)
    { route.points.clear(); route.next=0; route.retry=0; route.target=target; }
    if (route.next>=route.points.size())
    {
        const float length=Distance(position,target);
        if (length<.02f) return false;
        const float fraction=std::min(distance,length)/length;
        const Vec2 next={position.x+(target.x-position.x)*fraction,position.z+(target.z-position.z)*fraction};
        if (WorldLayout::CanTravel(world,position,next,.12f))
        { route.points.clear(); route.next=0; position=next; return true; }
        if (route.retry>0 || !Plan(world,position,target,route)) return false;
    }
    bool moved=false;
    while (distance>0 && route.next<route.points.size())
    {
        const Vec2 goal=route.points[route.next];
        const float length=Distance(position,goal);
        if (length<.02f) { ++route.next; continue; }
        const float amount=std::min(distance,length),fraction=amount/length;
        const Vec2 next={position.x+(goal.x-position.x)*fraction,position.z+(goal.z-position.z)*fraction};
        if (!WorldLayout::CanTravel(world,position,next,.12f))
        { route.points.clear(); route.next=0; route.retry=.5f; return moved; }
        position=next; distance-=amount; moved=true;
        if (amount>=length) ++route.next;
    }
    return moved;
}
}
