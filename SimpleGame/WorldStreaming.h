#pragma once
#include "GameContent.h"
#include <cmath>

namespace WorldStreaming
{
constexpr float ChunkSize=64;
inline std::pair<int,int> Key(Vec2 p)
{
    return {static_cast<int>(std::floor(p.x/ChunkSize)),static_cast<int>(std::floor(p.z/ChunkSize))};
}
inline bool Active(const GameState &s,Vec2 p)
{
    return s.loadedChunks.empty() || s.loadedChunks.count(Key(p))!=0;
}
inline void Update(GameState &s)
{
    const auto center=Key(s.player);
    if (center.first==s.streamX && center.second==s.streamZ) return;
    s.streamX=center.first; s.streamZ=center.second;
    for (auto it=s.loadedChunks.begin();it!=s.loadedChunks.end();)
        if (std::abs(it->first.first-center.first)>1 || std::abs(it->first.second-center.second)>1)
        { it=s.loadedChunks.erase(it); ++s.chunkUnloads; }
        else ++it;
    for (int z=center.second-1;z<=center.second+1;++z)
        for (int x=center.first-1;x<=center.first+1;++x)
        {
            const std::pair<int,int> key={x,z};
            if (s.loadedChunks.count(key)) continue;
            LoadedChunk chunk;
            const auto add=[&](Vec2 p,int kind,int index) { if (Key(p)==key) chunk.objects.push_back({kind,index}); };
            for (size_t i=0;i<s.npcs.size();++i) add(s.npcs[i].position,0,static_cast<int>(i));
            for (size_t i=0;i<s.animals.size();++i) add(s.animals[i].position,1,static_cast<int>(i));
            for (size_t i=0;i<s.gatherNodes.size();++i) add(s.gatherNodes[i].position,2,static_cast<int>(i));
            for (size_t i=0;i<s.fieldMonsters.size();++i) add(s.fieldMonsters[i].home,3,static_cast<int>(i));
            const auto &map=GameContent::Get().map;
            for (size_t i=0;i<map.houses.size();++i) add(map.houses[i].position,4,static_cast<int>(i));
            for (size_t i=0;i<map.trees.size();++i) add(map.trees[i].position,5,static_cast<int>(i));
            for (size_t i=0;i<map.landmarks.size();++i) add(map.landmarks[i].position,6,static_cast<int>(i));
            s.loadedChunks.emplace(key,std::move(chunk)); ++s.chunkLoads;
        }
}
}
