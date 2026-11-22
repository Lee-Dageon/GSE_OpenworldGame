#pragma once
#include "GameState.h"
#include <algorithm>
#include <numeric>
#include <cwchar>
#include <cmath>

namespace InventoryActions
{
inline std::array<int,static_cast<size_t>(ItemId::Count)> Order(const Inventory &inventory,int mode)
{
    std::array<int,static_cast<size_t>(ItemId::Count)> order;
    std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](int a,int b) {
        if (mode==1) return inventory.stacks[a]>inventory.stacks[b];
        if (mode==2) return std::wcscmp(ItemName(static_cast<ItemId>(a)),ItemName(static_cast<ItemId>(b)))<0;
        return a<b;
    });
    return order;
}
inline ItemId Selected(const GameState &s)
{
    const auto order=Order(s.inventory,s.inventorySort);
    return static_cast<ItemId>(order[std::clamp(s.inventorySelection,0,static_cast<int>(order.size())-1)]);
}
inline bool AtStorage(const GameState &s)
{
    return std::hypot(s.player.x-s.world.elder.x,s.player.z-s.world.elder.z)<3;
}
inline const wchar_t *Transfer(GameState &s,bool deposit,bool all)
{
    if (!s.inventoryOpen || !AtStorage(s)) return L"보관함은 장로 옆에서 가방을 열고 사용하세요.";
    const auto id=Selected(s);
    auto &from=deposit ? s.inventory : s.storage;
    auto &to=deposit ? s.storage : s.inventory;
    const int amount=all ? from.Count(id) : 1;
    if (amount<=0 || from.Count(id)<amount) return L"선택한 아이템이 없습니다.";
    if (!to.Add(id,amount)) return L"보관 한도를 넘습니다. 아이템은 그대로 있습니다.";
    from.Remove(id,amount);
    return deposit ? L"선택한 재료를 보관했습니다." : L"선택한 재료를 꺼냈습니다.";
}
inline const wchar_t *Discard(GameState &s,bool all)
{
    if (!s.inventoryOpen) return nullptr;
    const auto id=Selected(s);
    const int count=all ? s.inventory.Count(id) : 1;
    return s.inventory.Remove(id,count) ? L"선택한 재료를 폐기했습니다." : L"폐기할 아이템이 없습니다.";
}
inline int WeaponBonus(int tier) { return tier<0 ? 0 : tier==0 ? 3 : tier==1 ? 7 : 13; }
inline int ArmorBonus(int tier) { return tier<0 ? 0 : tier==0 ? 3 : tier==1 ? 6 : 11; }
inline void Equip(GameState &s,bool weapon,int tier)
{
    auto &equipped=weapon ? s.equippedWeapon : s.equippedArmor;
    const bool crafted=weapon ? s.hunterBladeCrafted : s.hunterArmorCrafted;
    const int highest=crafted ? (weapon ? s.weaponTier : s.armorTier) : -1;
    if (tier < -1 || tier>highest) return;
    if (weapon) s.attackPower+=WeaponBonus(tier)-WeaponBonus(equipped);
    else s.defense+=ArmorBonus(tier)-ArmorBonus(equipped);
    equipped=tier;
}
inline const wchar_t *CycleEquipment(GameState &s,bool weapon)
{
    if (!s.inventoryOpen) return nullptr;
    const int highest=(weapon ? s.hunterBladeCrafted : s.hunterArmorCrafted) ?
        (weapon ? s.weaponTier : s.armorTier) : -1;
    int next=(weapon ? s.equippedWeapon : s.equippedArmor)+1;
    if (next>highest) next=-1;
    Equip(s,weapon,next);
    return weapon ? L"검 장비를 교체했습니다." : L"방어구를 교체했습니다.";
}
inline const wchar_t *CraftStyles(GameState &s)
{
    if (!s.inventoryOpen || !AtStorage(s)) return L"장로 옆 가방에서 창·활을 제작하세요.";
    if (s.spearCrafted && s.bowCrafted) return L"창과 활을 모두 제작했습니다. Z로 무기를 전환하세요.";
    if (s.inventory.Count(ItemId::Hardwood)<4 || s.inventory.Count(ItemId::PlantFiber)<3 || s.inventory.Count(ItemId::BeastBone)<2)
        return L"창/활 제작: 목재 4 + 섬유 3 + 뼈 2가 필요합니다.";
    s.inventory.Remove(ItemId::Hardwood,4); s.inventory.Remove(ItemId::PlantFiber,3); s.inventory.Remove(ItemId::BeastBone,2);
    if (!s.spearCrafted) { s.spearCrafted=true; s.weaponStyle=1; return L"사냥 창 제작 · Z로 무기 전환 · 긴 사거리"; }
    s.bowCrafted=true; s.weaponStyle=2;
    return L"사냥 활 제작 · Z로 무기 전환 · 원거리 공격";
}
inline float Range(const GameState &s) { return s.weaponStyle==2 ? 10.0f : s.weaponStyle==1 ? 3.5f : 1.9f; }
}
