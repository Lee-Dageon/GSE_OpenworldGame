#include "stdafx.h"
#include "CraftingSystem.h"
#include <cmath>

namespace CraftingSystem
{
bool IsAtWorkbench(const GameState &state)
{
    const float x = state.player.x - state.world.elder.x, z = state.player.z - state.world.elder.z;
    return x*x + z*z < 1.8f*1.8f;
}

const wchar_t *TryCraft(GameState &state, Recipe recipe)
{
    if (!IsAtWorkbench(state))
        return L"장로 근처의 야영지 제작소에서 제작할 수 있습니다.";
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
        return L"엄니 검은 이미 제작하여 장착 중입니다.";
    if (state.quest != QuestStage::Complete)
        return L"먼저 장로에게 사냥 퀘스트를 보고하세요.";
    if (state.inventory.Count(ItemId::BoarHide) < 1 || state.inventory.Count(ItemId::BoarTusk) < 1)
        return L"엄니 검 제작에는 멧돼지 가죽 1개와 엄니 1개가 필요합니다.";
    state.inventory.Remove(ItemId::BoarHide);
    state.inventory.Remove(ItemId::BoarTusk);
    state.hunterBladeCrafted = true;
    state.attackPower += 3;
    return L"엄니 검 제작 완료! 자동 장착되어 공격력이 3 증가했습니다.";
}
}
