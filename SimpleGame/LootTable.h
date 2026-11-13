#pragma once
#include "Inventory.h"
#include "Monster.h"
#include <string>

namespace LootTable
{
using Drops = std::array<int, static_cast<size_t>(ItemId::Count)>;
inline Drops Animal(int species)
{
    Drops drops = {};
    drops[static_cast<size_t>(ItemId::RawMeat)] = species == 2 ? 1 : 2;
    if (species == 0)
    {
        drops[static_cast<size_t>(ItemId::BoarHide)] = 1;
        drops[static_cast<size_t>(ItemId::Antler)] = 1;
    }
    else if (species == 1)
    {
        drops[static_cast<size_t>(ItemId::BoarHide)] = 1;
        drops[static_cast<size_t>(ItemId::BoarTusk)] = 1;
    }
    else if (species == 2) drops[static_cast<size_t>(ItemId::Wool)] = 2;
    else if (species == 3)
    {
        drops.fill(0);
        drops[static_cast<size_t>(ItemId::LizardScale)] = 2;
        drops[static_cast<size_t>(ItemId::ReptileTail)] = 1;
    }
    else if (species == 4)
    {
        drops[static_cast<size_t>(ItemId::RawMeat)] = 3;
        drops[static_cast<size_t>(ItemId::CamelHide)] = 2;
        drops[static_cast<size_t>(ItemId::CamelMilk)] = 1;
    }
    else drops.fill(0);
    return drops;
}
inline Drops Monster(const MonsterDefinition &monster)
{
    Drops drops = {};
    drops[static_cast<size_t>(ItemId::BoarHide)] = monster.hideDrop;
    drops[static_cast<size_t>(ItemId::BoarTusk)] = monster.tuskDrop;
    if (monster.id == "ancient_guardian")
    {
        drops[static_cast<size_t>(ItemId::GuardianScale)] = 2;
        drops[static_cast<size_t>(ItemId::AncientCrystal)] = 1;
    }
    else
    {
        drops[static_cast<size_t>(ItemId::RawMeat)] = monster.id == "displaced_boar" ? 3 : 2;
        if (monster.id == "rampaging_boar" || monster.id == "displaced_boar")
            drops[static_cast<size_t>(ItemId::BeastBone)] = 2;
        else if (monster.id == "forest_boar") drops[static_cast<size_t>(ItemId::BeastBone)] = 1;
    }
    return drops;
}
inline bool Grant(Inventory &inventory, const Drops &drops)
{
    for (size_t index = 0; index < drops.size(); ++index)
        if (drops[index] < 0 || drops[index] > Inventory::MaxStack - inventory.Count(static_cast<ItemId>(index)))
            return false;
    for (size_t index = 0; index < drops.size(); ++index)
        if (drops[index] > 0) inventory.Add(static_cast<ItemId>(index), drops[index]);
    return true;
}
inline std::wstring Summary(const Drops &drops)
{
    std::wstring text;
    for (size_t index = 0; index < drops.size(); ++index)
        if (drops[index] > 0)
        {
            if (!text.empty()) text += L" · ";
            text += ItemName(static_cast<ItemId>(index));
            text += L" " + std::to_wstring(drops[index]);
        }
    return text;
}
}
