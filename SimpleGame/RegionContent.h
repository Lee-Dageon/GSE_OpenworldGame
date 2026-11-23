#pragma once
#include "GameContent.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace RegionContent
{
struct Descriptor { std::string id,file; float minX=0,minZ=0,maxX=0,maxZ=0; };
inline bool Id(const std::string &s)
{
    if (s.empty() || s.size()>64) return false;
    return std::all_of(s.begin(),s.end(),[](unsigned char c){ return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_'; });
}
inline std::wstring Text(const std::string &s)
{
    if (s.empty() || s.size()>2048) throw std::runtime_error("invalid regional text");
    const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);
    if (n<=0) throw std::runtime_error("invalid regional UTF-8");
    std::wstring result(n,L' ');
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),&result[0],n);
    return result;
}
inline void End(std::istringstream &row)
{
    if (!row) throw std::runtime_error("missing regional field");
    row>>std::ws;
    if (!row.eof()) throw std::runtime_error("extra regional field");
}
inline std::vector<std::string> Lines(const std::string &path)
{
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if (!in || in.tellg()>262144) throw std::runtime_error("cannot read regional file: "+path);
    in.seekg(0); std::vector<std::string> result; std::string line;
    while (std::getline(in,line))
    {
        if (line.size()>4096) throw std::runtime_error("regional line too long");
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (!line.empty() && line[0]!='#') result.push_back(line);
    }
    if (!in.eof() || result.empty()) throw std::runtime_error("incomplete regional file");
    return result;
}
inline std::vector<Descriptor> Catalog(const std::string &directory="./Data/Regions")
{
    if (!std::filesystem::exists(directory+"/index.gse")) return {};
    const auto lines=Lines(directory+"/index.gse");
    if (lines.front()!="gse_regions 1") throw std::runtime_error("unsupported region index");
    std::vector<Descriptor> entries; std::set<std::string> ids;
    for (size_t i=1;i<lines.size();++i)
    {
        Descriptor d; std::string tag; std::istringstream row(lines[i]);
        row>>tag>>d.id>>d.file>>d.minX>>d.minZ>>d.maxX>>d.maxZ; End(row);
        if (tag!="region" || !Id(d.id) || !ids.insert(d.id).second || entries.size()>=32 ||
            d.file!=d.id+".gse" || !std::isfinite(d.minX) || !std::isfinite(d.minZ) ||
            !std::isfinite(d.maxX) || !std::isfinite(d.maxZ) || d.minX>=d.maxX || d.minZ>=d.maxZ)
            throw std::runtime_error("invalid region descriptor");
        entries.push_back(d);
    }
    return entries;
}
inline RegionRuntime Load(const Descriptor &d,const std::string &directory="./Data/Regions")
{
    const auto lines=Lines(directory+"/"+d.file);
    std::istringstream header(lines.front()); std::string tag,id,title; int version=0;
    header>>tag>>version>>id>>std::quoted(title); End(header);
    if (tag!="gse_region" || version!=1 || id!=d.id) throw std::runtime_error("unsupported region payload");
    RegionRuntime r; r.id=id; r.name=Text(title); r.minX=d.minX; r.minZ=d.minZ; r.maxX=d.maxX; r.maxZ=d.maxZ;
    std::set<std::string> ids;
    const auto point=[&](Vec2 p) {
        if (!std::isfinite(p.x) || !std::isfinite(p.z) || p.x<d.minX || p.x>d.maxX || p.z<d.minZ || p.z>d.maxZ ||
            p.x<GameContent::Get().map.world.minimumCoordinate || p.x>GameContent::Get().map.world.maximumCoordinate ||
            p.z<GameContent::Get().map.world.minimumCoordinate || p.z>GameContent::Get().map.world.maximumCoordinate)
            throw std::runtime_error("invalid regional position");
    };
    for (size_t i=1;i<lines.size();++i)
    {
        std::istringstream row(lines[i]); row>>tag>>id;
        if (!Id(id) || !ids.insert(id).second || ids.size()>512) throw std::runtime_error("duplicate regional ID");
        if (tag=="site")
        {
            RegionSite site; std::string name,dialogue; site.id=id;
            row>>site.kind>>site.position.x>>site.position.z>>std::quoted(name)>>std::quoted(dialogue); End(row); point(site.position);
            if (site.kind<0 || site.kind>5) throw std::runtime_error("invalid regional site kind");
            site.name=Text(name); site.dialogue=Text(dialogue); r.sites.push_back(site);
        }
        else if (tag=="enemy")
        {
            RegionEnemy enemy; std::string monster; enemy.id=id;
            row>>monster>>enemy.home.x>>enemy.home.z; End(row); point(enemy.home);
            const auto found=GameContent::Get().monsters.find(monster);
            if (found==GameContent::Get().monsters.end()) throw std::runtime_error("unknown regional monster");
            enemy.monster=MonsterSystem::Create(found->second,enemy.home); r.enemies.push_back(enemy);
        }
        else if (tag=="gather")
        {
            RegionGather node; node.id=id;
            row>>node.item>>node.position.x>>node.position.z>>node.amount; End(row); point(node.position);
            if (node.item<0 || node.item>=static_cast<int>(ItemId::Count) || node.amount<1 || node.amount>10)
                throw std::runtime_error("invalid regional gather");
            r.gathers.push_back(node);
        }
        else if (tag=="goal")
        {
            RegionGoal goal; std::string objective,message; goal.id=id;
            row>>goal.action>>goal.target>>goal.next>>std::quoted(objective)>>std::quoted(message); End(row);
            if (goal.action!="travel" && goal.action!="interact" && goal.action!="hunt" && goal.action!="collect" &&
                goal.action!="craft" && goal.action!="report") throw std::runtime_error("unsupported regional goal");
            goal.objective=Text(objective); goal.message=Text(message); r.goals.push_back(goal);
        }
        else throw std::runtime_error("unknown regional record");
    }
    if (r.goals.empty() || r.goals.size()>64) throw std::runtime_error("incomplete regional story");
    std::set<std::string> visited;
    for (size_t i=0;i<r.goals.size();++i)
    {
        const auto &g=r.goals[i];
        const bool enemy=g.action=="hunt" || g.action=="collect";
        const bool found=enemy ? std::any_of(r.enemies.begin(),r.enemies.end(),[&](const RegionEnemy &e){return e.id==g.target;}) :
            std::any_of(r.sites.begin(),r.sites.end(),[&](const RegionSite &s){return s.id==g.target;});
        if (!found || g.next!=(i+1<r.goals.size() ? r.goals[i+1].id : "end") || !visited.insert(g.id).second)
            throw std::runtime_error("invalid regional goal references/order");
        if (g.action=="hunt" && (i+1==r.goals.size() || r.goals[i+1].action!="collect" || r.goals[i+1].target!=g.target))
            throw std::runtime_error("regional hunt must lead to collection");
    }
    return r;
}
}
