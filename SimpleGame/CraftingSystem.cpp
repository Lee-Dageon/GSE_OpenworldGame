#include "stdafx.h"
#include "CraftingSystem.h"
#include "NpcSystem.h"
#include "InventoryActions.h"
#include <cmath>

namespace
{
const wchar_t *UpgradeEquipment(GameState &state, bool weapon)
{
    int &tier = weapon ? state.weaponTier : state.armorTier;
    if (tier >= 2) return L"고대 장비는 최종 강화 상태입니다.";
    const ItemId first = tier == 0 ? ItemId::IronOre : ItemId::GuardianScale;
    const ItemId second = tier == 0 ? ItemId::Coal : ItemId::AncientCrystal;
    const int firstCost = tier == 0 ? 3 : 4;
    if (state.inventory.Count(first) < firstCost || state.inventory.Count(second) < 2)
        return tier == 0 ? L"철 장비 강화: 철광석 3 + 석탄 2가 필요합니다." :
            L"고대 장비 강화: 수호자 갑각 4 + 고대 결정 2가 필요합니다.";
    state.inventory.Remove(first, firstCost);
    state.inventory.Remove(second, 2);
    ++tier;
    InventoryActions::Equip(state,weapon,tier);
    return weapon ? (tier == 1 ? L"철 이빨 검 장착 · 공격력 +4" : L"고대 결정 검 장착 · 공격력 +6") :
        (tier == 1 ? L"철 보강 방어구 장착 · 방어력 +3" : L"고대 갑각 방어구 장착 · 방어력 +5");
}
}

namespace CraftingSystem
{
bool IsAtWorkbench(const GameState &state)
{
    const float x = state.player.x - state.world.elder.x, z = state.player.z - state.world.elder.z;
    if (x*x + z*z < 1.8f*1.8f) return true;
    const int index=NpcSystem::Nearby(state);
    return index>=0 && (NpcSystem::Role(state.npcs[index])==1 || NpcSystem::Role(state.npcs[index])==2);
}

const wchar_t *TryCraft(GameState &state, Recipe recipe)
{
    if (!IsAtWorkbench(state))
        return L"야영지·대장장이·약사 근처에서 제작할 수 있습니다.";
    if (recipe == Recipe::HunterArmor)
    {
        if (state.hunterArmorCrafted) return UpgradeEquipment(state, false);
        if (!state.armorRecipeUnlocked) return L"첫 본 이야기 사냥을 보고하면 방어구 제작법을 배웁니다.";
        if (state.inventory.Count(ItemId::BoarHide) < 2 || state.inventory.Count(ItemId::BoarTusk) < 1)
            return L"가죽 방어구 제작에는 가죽 2개와 이빨 1개가 필요합니다.";
        state.inventory.Remove(ItemId::BoarHide,2);
        state.inventory.Remove(ItemId::BoarTusk);
        state.hunterArmorCrafted = true;
        InventoryActions::Equip(state,false,0);
        return L"가죽 방어구를 제작해 장착했습니다. 받는 피해가 3 감소합니다.";
    }
    if (recipe == Recipe::HealingPotion)
    {
        if (state.inventory.Count(ItemId::Herb) < 1)
            return L"회복 물약 제작에는 약초 1개가 필요합니다.";
        if (!state.inventory.Add(ItemId::Potion))
            return L"물약 보관 한도(99개)에 도달했습니다.";
        state.inventory.Remove(ItemId::Herb);
        return L"회복 물약 1개를 제작했습니다.";
    }
    if (recipe != Recipe::HunterBlade)
        return L"알 수 없는 제작법입니다.";
    if (state.hunterBladeCrafted)
        return UpgradeEquipment(state, true);
    if (state.quest != QuestStage::Complete)
        return L"먼저 장로에게 사냥 퀘스트를 보고하세요.";
    if (state.inventory.Count(ItemId::BoarHide) < 1 || state.inventory.Count(ItemId::BoarTusk) < 1)
        return L"이빨 검 제작에는 멧돼지 가죽 1개와 이빨 1개가 필요합니다.";
    state.inventory.Remove(ItemId::BoarHide);
    state.inventory.Remove(ItemId::BoarTusk);
    state.hunterBladeCrafted = true;
    InventoryActions::Equip(state,true,0);
    return L"이빨 검 제작 완료! 자동 장착되어 공격력이 3 증가했습니다.";
}
}
