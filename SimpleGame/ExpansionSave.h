#pragma once
#include "RegionSystem.h"

namespace SaveGame
{
struct LegacySections
{
    std::vector<std::string> prefix,world;
    std::array<std::vector<std::string>,4> records;
    int version=0;
};
inline bool BaseId(const std::string &id)
{
    return !id.empty() && id.size()<=64 && std::all_of(id.begin(),id.end(),[](unsigned char c) {
        return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-' || c=='.';
    });
}
inline bool SplitLegacy(const std::string &bytes,LegacySections &s)
{
    std::istringstream in(bytes); std::string line;
    for (int i=0;i<6;++i) { if (!std::getline(in,line) || line.size()>8192) return false; s.prefix.push_back(line); }
    std::istringstream header(s.prefix[0]); std::string magic; header>>magic>>s.version;
    if (!header || magic!="gse_save" || (s.version!=1 && s.version!=2)) return false;
    const auto block=[&](int index) {
        if (!std::getline(in,line)) return false;
        std::istringstream countLine(line); size_t count=0; if (!(countLine>>count)) return false; countLine>>std::ws;
        if (!countLine.eof() || count>8192) return false;
        std::set<std::string> ids;
        for (size_t i=0;i<count;++i)
        {
            if (!std::getline(in,line) || line.size()>4096) return false;
            std::istringstream row(line); std::string id; row>>id;
            if (!BaseId(id) || !ids.insert(id).second) return false;
            s.records[index].push_back(line);
        }
        return true;
    };
    for (int i=0;i<3;++i) if (!block(i)) return false;
    if (s.version==2)
    {
        for (int i=0;i<10;++i) { if (!std::getline(in,line) || line.size()>8192) return false; s.world.push_back(line); }
        if (!block(3)) return false;
    }
    if (!std::getline(in,line) || line!="end") return false;
    in>>std::ws; return in.eof();
}
inline std::vector<std::string> RemapRecords(const std::string &kind,const std::vector<std::string> &saved,
    const std::vector<std::string> &defaults,std::map<std::string,std::string> &archive)
{
    for (const auto &row:saved)
    { std::istringstream in(row); std::string id; in>>id; archive[kind+"/"+id]=row; }
    std::vector<std::string> result;
    for (const auto &row:defaults)
    {
        std::istringstream in(row); std::string id; in>>id;
        const auto found=archive.find(kind+"/"+id);
        result.push_back(found==archive.end() ? row : found->second);
    }
    return result;
}
inline bool PadStacks(std::string &line,bool counted)
{
    std::istringstream in(line); size_t count=static_cast<size_t>(ItemId::Count);
    if (counted) in>>count;
    if (!in || count>256) return false;
    std::vector<int> values; int value=0;
    while (in>>value) { if (value<0 || value>Inventory::MaxStack) return false; values.push_back(value); }
    if (!in.eof() || (counted && values.size()!=count) || values.size()>static_cast<size_t>(ItemId::Count)) return false;
    values.resize(static_cast<size_t>(ItemId::Count),0);
    std::ostringstream out; if (counted) out<<values.size();
    for (size_t i=0;i<values.size();++i) out<<((counted || i) ? " " : "")<<values[i];
    line=out.str(); return true;
}
inline bool NormalizeLegacy(const std::string &bytes,std::map<std::string,std::string> &archive,std::string &normalized)
{
    LegacySections old,defaults;
    if (!SplitLegacy(bytes,old) || !SplitLegacy(EncodeLegacy(CreateInitialGameState()),defaults)) return false;
    if (!PadStacks(old.prefix[4],true)) return false;
    {
        std::istringstream stats(old.prefix[2]); float x=0,z=0; stats>>x>>z;
        const auto &map=GameContent::Get().map;
        if (stats && std::isfinite(x) && std::isfinite(z) && x>=map.world.minimumCoordinate && x<=map.world.maximumCoordinate &&
            z>=map.world.minimumCoordinate && z<=map.world.maximumCoordinate && (GameContent::IsBlocked(map,{x,z}) ||
            std::hypot(x-map.world.lake.x,z-map.world.lake.z)<map.world.lakeRadius))
        {
            // New map geometry can cover an old save position; keep progress and relocate safely.
            std::string rest; std::getline(stats,rest);
            std::ostringstream safe; safe<<map.world.spawn.x<<' '<<map.world.spawn.z<<rest; old.prefix[2]=safe.str();
        }
    }
    const char *kinds[]={"gather","animal","npc","field"};
    for (int i=0;i<(old.version==2 ? 4 : 3);++i)
    {
        old.records[i]=RemapRecords(kinds[i],old.records[i],defaults.records[i],archive);
        if (i==2)
            for (auto &row:old.records[i])
            {
                std::istringstream fields(row); std::vector<std::string> words; std::string word;
                while (fields>>word) words.push_back(word);
                if (old.version==1 && words.size()==5) row=row.substr(0,row.find_last_of(' '));
                if (old.version==2 && words.size()==4) row+=" 100";
            }
    }
    if (old.version==2 && !PadStacks(old.world[1],false)) return false;
    std::ostringstream out;
    for (const auto &row:old.prefix) out<<row<<'\n';
    for (int i=0;i<3;++i) { out<<old.records[i].size()<<'\n'; for (const auto &row:old.records[i]) out<<row<<'\n'; }
    if (old.version==2)
    {
        for (const auto &row:old.world) out<<row<<'\n';
        out<<old.records[3].size()<<'\n'; for (const auto &row:old.records[3]) out<<row<<'\n';
    }
    out<<"end\n"; normalized=out.str(); return true;
}
inline std::string Encode(const GameState &s)
{
    const auto core=EncodeLegacy(s);
    std::ostringstream out; out.imbue(std::locale::classic()); out<<std::setprecision(17);
    out<<"gse_save 3\ncore "<<core.size()<<'\n'<<core;
    out<<"expansion "<<s.prologueLossApplied<<' '<<s.lostWeaponTier<<' '<<s.lostArmorTier<<' '
        <<s.trackingActive<<' '<<s.trackingStep<<' '<<s.trackingCompleted<<' '
        <<s.variantResearchActive<<' '<<s.variantSamples<<' '<<s.variantResearchCompleted<<' '
        <<s.festivalRound<<' '<<s.festivalStage<<' '<<s.festivalScore<<' '<<s.festivalFinished<<' '
        <<s.worldEventStage<<' '<<s.worldEventKills<<' '<<s.worldEventCompleted<<' '<<s.worldEventTime<<' '<<s.worldEventDamage<<'\n';
    out<<"samples "<<s.variantSurveyed.size()<<'\n';
    for (const auto &id:s.variantSurveyed) out<<std::quoted(id)<<'\n';
    auto progress=s.regionProgress;
    for (const auto &r:s.regions)
    { RegionSystem::Snapshot(r.second,progress[r.first]); progress[r.first].lastWorldTime=s.worldTime; }
    out<<"regions "<<progress.size()<<'\n';
    for (const auto &r:progress)
    {
        const auto &p=r.second;
        out<<std::quoted(r.first)<<' '<<std::quoted(p.stepId)<<' '<<p.started<<' '<<p.finished<<' '<<p.kills<<' '<<p.lastWorldTime<<' '<<p.objects.size()<<'\n';
        for (const auto &o:p.objects)
            out<<std::quoted(o.first)<<' '<<o.second.position.x<<' '<<o.second.position.z<<' '<<o.second.health<<' '
                <<o.second.timer<<' '<<o.second.collected<<' '<<o.second.rewarded<<'\n';
    }
    out<<"raid "<<s.eventMonsters.size()<<'\n';
    for (const auto &f:s.eventMonsters)
        out<<f.monster.definition.id<<' '<<f.home.x<<' '<<f.home.z<<' '<<f.monster.position.x<<' '<<f.monster.position.z<<' '
            <<f.monster.health<<' '<<f.rewardGranted<<'\n';
    out<<"archive "<<s.orphanRecords.size()<<'\n';
    for (const auto &record:s.orphanRecords) out<<std::quoted(record.first)<<' '<<std::quoted(record.second)<<'\n';
    out<<"end_expansion\n";
    if (!out) throw std::runtime_error("expansion encoding failed");
    return out.str();
}
inline bool Decode(const std::string &bytes,const GameState &current,GameState &result)
{
    if (bytes.empty() || bytes.size()>4*1024*1024) return false;
    const bool v3=bytes.rfind("gse_save 3\n",0)==0;
    std::istringstream in(bytes); in.imbue(std::locale::classic()); std::string core=bytes,tag;
    if (v3)
    {
        std::string header; std::getline(in,header); size_t size=0; in>>tag>>size;
        if (!in || tag!="core" || size<16 || size>1024*1024 || in.get()!='\n') return false;
        core.resize(size); in.read(&core[0],static_cast<std::streamsize>(size)); if (!in) return false;
    }
    std::string normalized; std::map<std::string,std::string> archive;
    if (v3)
    {
        const auto marker=bytes.rfind("\narchive ");
        if (marker==std::string::npos) return false;
        std::istringstream records(bytes.substr(marker+1)); size_t count=0; records>>tag>>count;
        if (!records || count>8192) return false;
        for (size_t i=0;i<count;++i)
        {
            std::string key,row; records>>std::quoted(key)>>std::quoted(row);
            if (!records || key.size()>160 || row.size()>4096 || row.find_first_of("\r\n")!=std::string::npos || !archive.emplace(key,row).second) return false;
        }
    }
    if (!NormalizeLegacy(core,archive,normalized)) return false;
    GameState s;
    if (!DecodeLegacy(normalized,current,s)) return false;
    if (v3)
    {
        in>>tag>>s.prologueLossApplied>>s.lostWeaponTier>>s.lostArmorTier>>s.trackingActive>>s.trackingStep>>s.trackingCompleted
            >>s.variantResearchActive>>s.variantSamples>>s.variantResearchCompleted>>s.festivalRound>>s.festivalStage>>s.festivalScore>>s.festivalFinished
            >>s.worldEventStage>>s.worldEventKills>>s.worldEventCompleted>>s.worldEventTime>>s.worldEventDamage;
        if (!in || tag!="expansion" || s.lostWeaponTier< -1 || s.lostWeaponTier>2 || s.lostArmorTier< -1 || s.lostArmorTier>2 ||
            s.trackingStep<0 || s.trackingStep>3 || s.trackingCompleted<0 || s.trackingCompleted>1000000 ||
            s.variantSamples<0 || s.variantSamples>2 || s.variantResearchCompleted<0 || s.variantResearchCompleted>1000000 ||
            s.festivalRound< -1 || s.festivalRound>1000000 || s.festivalFinished< -1 || s.festivalFinished>1000000 ||
            s.festivalStage<0 || s.festivalStage>4 || s.festivalScore<0 || s.festivalScore>7 ||
            s.worldEventStage<0 || s.worldEventStage>3 || s.worldEventKills<0 || s.worldEventKills>2 ||
            s.worldEventCompleted<0 || s.worldEventCompleted>1000000 || !Timer(s.worldEventTime) || s.worldEventDamage<0 || s.worldEventDamage>10000) return false;
        size_t count=0; in>>tag>>count; if (!in || tag!="samples" || count>256) return false;
        for (size_t i=0;i<count;++i) { std::string id; in>>std::quoted(id); if (!in || id.size()>160 || !s.variantSurveyed.insert(id).second) return false; }
        in>>tag>>count; if (!in || tag!="regions" || count>64) return false;
        for (size_t i=0;i<count;++i)
        {
            std::string id; RegionProgress p; size_t objects=0;
            in>>std::quoted(id)>>std::quoted(p.stepId)>>p.started>>p.finished>>p.kills>>p.lastWorldTime>>objects;
            if (!in || !RegionContent::Id(id) || (!p.stepId.empty() && !RegionContent::Id(p.stepId)) || p.kills<0 || p.kills>1000000 || !std::isfinite(p.lastWorldTime) || p.lastWorldTime<0 || p.lastWorldTime>1000000000.0 || objects>2048 || s.regionProgress.count(id)) return false;
            for (size_t j=0;j<objects;++j)
            {
                std::string key; RegionObjectProgress o;
                in>>std::quoted(key)>>o.position.x>>o.position.z>>o.health>>o.timer>>o.collected>>o.rewarded;
                if (!in || !RegionContent::Id(key) || !Point(o.position,s.world) || o.health< -1 || o.health>1000000 || !Timer(o.timer) || p.objects.count(key)) return false;
                p.objects.emplace(key,o);
            }
            s.regionProgress.emplace(id,std::move(p));
        }
        in>>tag>>count; if (!in || tag!="raid" || count>2) return false;
        for (size_t i=0;i<count;++i)
        {
            FieldMonster f; std::string id; Vec2 position; int health=0;
            in>>id>>f.home.x>>f.home.z>>position.x>>position.z>>health>>f.rewardGranted;
            if (!in || !GameContent::Get().monsters.count(id) || !Point(f.home,s.world) || !Point(position,s.world) || health<0 || health>1000000) return false;
            auto definition=GameContent::Get().monsters.at(id); definition.chaseRange=40;
            f.monster=MonsterSystem::Create(definition,position); if (health>f.monster.maxHealth) return false;
            f.monster.health=health; f.monster.deathTime=health==0 ? 4.0f : 0; f.monster.attackCooldown=1;
            s.eventMonsters.push_back(f);
        }
        in>>tag>>count; if (!in || tag!="archive" || count>8192) return false;
        for (size_t i=0;i<count;++i)
        {
            std::string key,row; in>>std::quoted(key)>>std::quoted(row);
            if (!in || key.size()>160 || row.size()>4096 || row.find_first_of("\r\n")!=std::string::npos) return false;
            // Live records from the core win; retired records remain for later reintroduction.
            archive.emplace(key,row);
        }
        in>>tag; if (!in || tag!="end_expansion") return false;
        in>>std::ws; if (!in.eof()) return false;
    }
    else s.festivalFinished=s.festivalClaimDay;
    s.orphanRecords=std::move(archive);
    result=std::move(s); return true;
}
}
