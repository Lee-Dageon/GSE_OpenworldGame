#pragma once
#include "GameContent.h"
#include "StorySystem.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace SaveGame
{
inline bool Point(Vec2 p, const WorldState &world)
{
    return std::isfinite(p.x) && std::isfinite(p.z) &&
        p.x >= world.minimumCoordinate && p.x <= world.maximumCoordinate &&
        p.z >= world.minimumCoordinate && p.z <= world.maximumCoordinate;
}
inline bool Timer(float value)
{
    return std::isfinite(value) && value >= 0 && value <= 1000000;
}
inline std::string Encode(const GameState &s)
{
    const auto &content = GameContent::Get();
    if (s.gatherNodes.size()!=content.map.gatherNodes.size() || s.animals.size()!=content.map.animals.size() ||
        s.npcs.size()!=content.map.npcs.size() || s.fieldMonsters.size()!=content.map.encounters.size()) throw std::runtime_error("world shape changed");
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(9) << "gse_save 2 " << content.map.id << ' ' << std::quoted(content.story.id) << '\n';
    const auto *step = StorySystem::Current(s);
    out << (step ? step->chapterId : "none") << ' ' << (step ? step->id : "none") << ' ' << s.storyFinished << '\n';
    out << s.player.x << ' ' << s.player.z << ' ' << s.facing.x << ' ' << s.facing.z << ' '
        << s.health << ' ' << s.maxHealth << ' ' << s.level << ' ' << s.experience << ' '
        << s.experienceToNextLevel << ' ' << s.attackPower << ' ' << s.defense << ' ' << static_cast<int>(s.quest) << '\n';
    out << s.hunterBladeCrafted << ' ' << s.hunterArmorCrafted << ' ' << s.armorRecipeUnlocked << ' '
        << s.weaponTier << ' ' << s.armorTier << ' ' << s.ruinDeviceDisabled << ' '
        << s.rescuedSurvivors << ' ' << s.migrationChoice << ' ' << s.endingChoice << ' '
        << s.completedHunts << ' ' << s.herbRegrowTime << ' ' << s.cameraZoom << '\n';
    out << s.inventory.stacks.size();
    for (int count : s.inventory.stacks) out << ' ' << count;
    const auto &m = s.monster;
    out << '\n' << m.definition.id << ' ' << m.position.x << ' ' << m.position.z << ' '
        << m.health << ' ' << m.maxHealth << ' ' << m.deathTime << '\n';
    out << s.gatherNodes.size() << '\n';
    for (size_t i = 0; i < s.gatherNodes.size(); ++i)
        out << content.map.gatherNodes[i].id << ' ' << s.gatherNodes[i].regrowTime << '\n';
    out << s.animals.size() << '\n';
    for (size_t i = 0; i < s.animals.size(); ++i)
    {
        const auto &a = s.animals[i];
        out << content.map.animals[i].id << ' ' << a.position.x << ' ' << a.position.z << ' '
            << a.health << ' ' << a.collected << ' ' << a.respawnTime << ' ' << a.deathTime << '\n';
    }
    out << s.npcs.size() << '\n';
    for (size_t i = 0; i < s.npcs.size(); ++i)
        out << content.map.npcs[i].id << ' ' << s.npcs[i].position.x << ' ' << s.npcs[i].position.z << ' '
            << s.npcs[i].conversations << ' ' << s.npcs[i].health << '\n';
    out << "world " << s.worldTime << ' ' << s.farmTime << ' ' << s.coins << ' ' << s.festivalClaimDay << ' '
        << s.regionVisits << ' ' << s.nextBaseUpgrade << ' ' << s.prologueSeen << ' '
        << s.equippedWeapon << ' ' << s.equippedArmor << ' ' << s.weaponStyle << ' '
        << s.spearCrafted << ' ' << s.bowCrafted << ' ' << s.companion.x << ' ' << s.companion.z << '\n';
    for (int count:s.storage.stacks) out << count << ' ';
    out << '\n';
    for (int level:s.baseLevels) out << level << ' ';
    out << '\n';
    for (const auto &c:s.contracts) out << c.active << ' ' << c.progress << ' ' << c.completed << '\n';
    for (const auto &e:s.escorts) out << e.following << ' ' << e.arrived << ' ' << e.position.x << ' ' << e.position.z << '\n';
    out << s.fieldMonsters.size() << '\n';
    for (size_t i=0;i<s.fieldMonsters.size();++i)
    {
        const auto &f=s.fieldMonsters[i];
        out << content.map.encounters[i].id << ' ' << f.monster.definition.id << ' '
            << f.monster.position.x << ' ' << f.monster.position.z << ' ' << f.monster.health << ' '
            << f.monster.deathTime << ' ' << f.collected << ' ' << f.rewardGranted << ' ' << f.respawnTime << '\n';
    }
    out << "end\n";
    if (!out) throw std::runtime_error("save encoding failed");
    return out.str();
}

// Parse into a separate state. No caller state changes on any rejection.
inline bool Decode(const std::string &bytes, const GameState &current, GameState &result)
{
    if (bytes.empty() || bytes.size() > 1024*1024) return false;
    std::istringstream in(bytes);
    in.imbue(std::locale::classic());
    const auto &content = GameContent::Get();
    std::string magic, mapId, storyId, chapterId, stepId, id;
    int version = 0, quest = 0;
    GameState s = CreateInitialGameState();
    in >> magic >> version >> mapId >> std::quoted(storyId);
    if (!in || magic != "gse_save" || (version != 1 && version != 2) || mapId != content.map.id || storyId != content.story.id) return false;
    in >> chapterId >> stepId >> s.storyFinished;
    if (chapterId == "none" && stepId == "none")
        s.storyStep = s.storyFinished ? static_cast<int>(content.story.steps.size()) : -1;
    else
    {
        if (s.storyFinished || !StorySystem::SetProgressById(s, chapterId, stepId)) return false;
    }
    in >> s.player.x >> s.player.z >> s.facing.x >> s.facing.z >> s.health >> s.maxHealth >> s.level
        >> s.experience >> s.experienceToNextLevel >> s.attackPower >> s.defense >> quest;
    in >> s.hunterBladeCrafted >> s.hunterArmorCrafted >> s.armorRecipeUnlocked >> s.weaponTier >> s.armorTier
        >> s.ruinDeviceDisabled >> s.rescuedSurvivors >> s.migrationChoice >> s.endingChoice
        >> s.completedHunts >> s.herbRegrowTime >> s.cameraZoom;
    if (!in || !Point(s.player,s.world) || GameContent::IsBlocked(content.map,s.player) ||
        !std::isfinite(s.facing.x) || !std::isfinite(s.facing.z) || std::abs(s.facing.x)>100 || std::abs(s.facing.z)>100 ||
        !std::isfinite(s.health) || !std::isfinite(s.maxHealth) || s.health<=0 || s.health>s.maxHealth || s.maxHealth>100000 ||
        s.level<1 || s.level>10000 || s.experience<0 || s.experienceToNextLevel<1 || s.experienceToNextLevel>1000000 ||
        s.experience>=s.experienceToNextLevel || s.attackPower<1 || s.attackPower>100000 || s.defense<0 || s.defense>100000 ||
        quest<0 || quest>6 || s.weaponTier<0 || s.weaponTier>2 || s.armorTier<0 || s.armorTier>2 ||
        (s.weaponTier>0 && !s.hunterBladeCrafted) || (s.armorTier>0 && !s.hunterArmorCrafted) ||
        s.rescuedSurvivors<0 || s.rescuedSurvivors>2 || s.migrationChoice<0 || s.migrationChoice>2 ||
        s.endingChoice<0 || s.endingChoice>2 || (s.endingChoice!=0 && !s.storyFinished) ||
        s.completedHunts<0 || s.completedHunts>1000000 || !Timer(s.herbRegrowTime) ||
        !std::isfinite(s.cameraZoom) || s.cameraZoom<GameConstants::World::ZoomMinimum ||
        s.cameraZoom>GameConstants::World::ZoomMaximum) return false;
    s.quest = static_cast<QuestStage>(quest);
    const float lakeX=s.player.x-s.world.lake.x, lakeZ=s.player.z-s.world.lake.z;
    if (lakeX*lakeX+lakeZ*lakeZ<s.world.lakeRadius*s.world.lakeRadius) return false;
    if (s.player.x>=790 && s.player.z>=620 && s.endingChoice==0) return false;
    size_t count = 0;
    in >> count;
    if (!in || count != s.inventory.stacks.size()) return false;
    for (int &stack : s.inventory.stacks)
    {
        in >> stack;
        if (!in || stack<0 || stack>Inventory::MaxStack) return false;
    }
    Vec2 position = {};
    int health = 0, maxHealth = 0;
    float deathTime = 0;
    in >> id >> position.x >> position.z >> health >> maxHealth >> deathTime;
    if (!in || !content.monsters.count(id) || !Point(position,s.world) || health<0 || health>maxHealth ||
        maxHealth<1 || maxHealth>1000000 || !Timer(deathTime)) return false;
    s.monster = MonsterSystem::Create(content.monsters.at(id),position);
    s.monster.health=health; s.monster.maxHealth=maxHealth; s.monster.deathTime=deathTime;
    s.monster.attackCooldown=1.0f;
    const auto *step = StorySystem::Current(s);
    if (version==1 && step && (step->id=="forest_hunt" || step->id=="forest_loot") && id=="displaced_boar")
    {
        id="forest_predator";
        s.monster=MonsterSystem::Create(content.monsters.at(id),position);
        s.monster.health=health==0 ? 0 : std::max(1,health*s.monster.maxHealth/maxHealth);
        s.monster.deathTime=deathTime; s.monster.attackCooldown=1;
        health=s.monster.health;
    }
    if (step && step->action == GameContent::StoryAction::Hunt && (health==0 || id!=step->monsterId)) return false;
    if (step && step->action == GameContent::StoryAction::Collect &&
        (health!=0 || s.storyStep<1 || id!=content.story.steps[s.storyStep-1].monsterId)) return false;
    if (step && (s.quest!=QuestStage::Complete || !s.hunterBladeCrafted)) return false;
    in >> count;
    if (!in || count!=s.gatherNodes.size()) return false;
    for (size_t i=0; i<count; ++i)
    {
        in >> id >> s.gatherNodes[i].regrowTime;
        if (!in || id!=content.map.gatherNodes[i].id || !Timer(s.gatherNodes[i].regrowTime)) return false;
    }
    in >> count;
    if (!in || count!=s.animals.size()) return false;
    for (size_t i=0; i<count; ++i)
    {
        auto &a=s.animals[i];
        in >> id >> a.position.x >> a.position.z >> a.health >> a.collected >> a.respawnTime >> a.deathTime;
        if (!in || id!=content.map.animals[i].id || !Point(a.position,s.world) || a.health<0 ||
            a.health>GameConstants::World::AnimalHealths[std::max(0,std::min(4,a.column))] || !Timer(a.respawnTime) || !Timer(a.deathTime) ||
            (a.collected && a.health>0)) return false;
    }
    in >> count;
    if (!in || count!=s.npcs.size()) return false;
    for (size_t i=0; i<count; ++i)
    {
        auto &npc=s.npcs[i];
        in >> id >> npc.position.x >> npc.position.z >> npc.conversations;
        if (version==2) in >> npc.health;
        if (!in || id!=content.map.npcs[i].id || !Point(npc.position,s.world) ||
            npc.conversations<0 || npc.conversations>1000000 || !std::isfinite(npc.health) || npc.health<0 || npc.health>100) return false;
        npc.target=npc.position;
    }
    if (version==2)
    {
        in >> id;
        if (!in || id!="world") return false;
        in >> s.worldTime >> s.farmTime >> s.coins >> s.festivalClaimDay >> s.regionVisits >> s.nextBaseUpgrade >> s.prologueSeen
            >> s.equippedWeapon >> s.equippedArmor >> s.weaponStyle >> s.spearCrafted >> s.bowCrafted >> s.companion.x >> s.companion.z;
        if (!in || !Timer(s.worldTime) || !Timer(s.farmTime) || s.coins<0 || s.coins>10000000 ||
            s.festivalClaimDay< -1 || s.festivalClaimDay>1000000 || s.regionVisits<0 || s.regionVisits>65535 ||
            s.nextBaseUpgrade<0 || s.nextBaseUpgrade>3 || s.equippedWeapon< -1 || s.equippedArmor< -1 ||
            s.equippedWeapon>(s.hunterBladeCrafted ? s.weaponTier : -1) ||
            s.equippedArmor>(s.hunterArmorCrafted ? s.armorTier : -1) || s.weaponStyle<0 || s.weaponStyle>2 ||
            (s.weaponStyle==1 && !s.spearCrafted) || (s.weaponStyle==2 && !s.bowCrafted) || !Point(s.companion,s.world)) return false;
        for (auto &stack:s.storage.stacks) { in >> stack; if (!in || stack<0 || stack>Inventory::MaxStack) return false; }
        for (auto &level:s.baseLevels) { in >> level; if (!in || level<0 || level>3) return false; }
        constexpr int targets[]={2,6,3,1,2};
        for (size_t i=0;i<s.contracts.size();++i)
        {
            auto &c=s.contracts[i]; in >> c.active >> c.progress >> c.completed;
            if (!in || c.progress<0 || c.progress>targets[i] || c.completed<0 || c.completed>1000000) return false;
        }
        for (auto &e:s.escorts)
        {
            in >> e.following >> e.arrived >> e.position.x >> e.position.z;
            if (!in || !Point(e.position,s.world) || (e.following && e.arrived)) return false;
        }
        in >> count;
        if (!in || count!=s.fieldMonsters.size()) return false;
        for (size_t i=0;i<count;++i)
        {
            auto &f=s.fieldMonsters[i]; std::string monsterId;
            in >> id >> monsterId >> f.monster.position.x >> f.monster.position.z >> f.monster.health
                >> f.monster.deathTime >> f.collected >> f.rewardGranted >> f.respawnTime;
            if (!in || id!=content.map.encounters[i].id || monsterId!=content.map.encounters[i].monsterId ||
                !Point(f.monster.position,s.world) || f.monster.health<0 || f.monster.health>f.monster.maxHealth ||
                !Timer(f.monster.deathTime) || !Timer(f.respawnTime) ||
                (f.collected && f.monster.health>0) || (f.rewardGranted && f.monster.health>0)) return false;
            f.monster.attackCooldown=1;
        }
    }
    else
    {
        s.equippedWeapon=s.hunterBladeCrafted ? s.weaponTier : -1;
        s.equippedArmor=s.hunterArmorCrafted ? s.armorTier : -1;
        s.prologueSeen=true;
        if (step && step->action==GameContent::StoryAction::Rescue)
            s.escorts[step->id=="rescue_scout" ? 1 : 0].position=step->position;
    }
    in >> id;
    if (!in || id!="end") return false;
    in >> std::ws;
    if (!in.eof()) return false;
    s.width=current.width; s.height=current.height; s.lastTime=current.lastTime;
    s.time=current.time;
    result=std::move(s);
    return true;
}

inline const wchar_t *Write(const GameState &state)
{
    try
    {
        const auto bytes=Encode(state);
        GameState checked;
        if (!Decode(bytes,state,checked)) return L"현재 상태를 저장할 수 없습니다. 기존 저장은 보존했습니다.";
        std::filesystem::create_directories("./Saves");
        const std::filesystem::path temporary=L"./Saves/progress.tmp";
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out) return L"저장 쓰기 실패. 기존 저장은 보존했습니다.";
        out.close();
        if (!out) return L"저장 마무리 실패. 기존 저장은 보존했습니다.";
        const wchar_t *path=L"./Saves/progress.gse";
        const bool exists=std::filesystem::exists(path);
        if (exists ? !ReplaceFileW(path,temporary.c_str(),L"./Saves/progress.bak",0,nullptr,nullptr) :
            !MoveFileExW(temporary.c_str(),path,MOVEFILE_WRITE_THROUGH))
            return L"저장 교체 실패. 기존 저장과 백업을 확인하세요.";
        return L"진행 저장 완료 · L 불러오기 · Saves/progress.gse";
    }
    catch (const std::exception &) { return L"저장 실패. 기존 저장을 덮어쓰지 않았습니다."; }
}
inline const wchar_t *Read(GameState &state)
{
    try
    {
        std::ifstream in("./Saves/progress.gse",std::ios::binary|std::ios::ate);
        if (!in) return L"저장 파일이 없습니다. K로 먼저 저장하세요.";
        const auto size=in.tellg();
        if (size<=0 || size>1024*1024) return L"저장 크기가 잘못되었습니다. 현재 진행을 유지합니다.";
        std::string bytes(static_cast<size_t>(size),'\0');
        in.seekg(0); in.read(&bytes[0],static_cast<std::streamsize>(bytes.size()));
        GameState loaded;
        if (!in || !Decode(bytes,state,loaded)) return L"손상되거나 호환되지 않는 저장입니다. 현재 진행을 유지합니다.";
        state=std::move(loaded);
        return L"진행 불러오기 완료 · 보유품·장비·선택·월드 상태 복원";
    }
    catch (const std::exception &) { return L"불러오기 실패. 현재 진행을 유지합니다."; }
}
}
