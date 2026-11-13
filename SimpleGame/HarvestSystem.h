#pragma once
#include "GameState.h"
#include <algorithm>
#include <cmath>

namespace HarvestSystem
{
constexpr float Range = 2.2f;
inline bool IsOre(int kind) { return kind >= 4 && kind <= 11; }
inline ItemId Item(const GatherNodeState &node) { return static_cast<ItemId>(static_cast<int>(ItemId::WildBerry) + node.kind); }
inline float Duration(const GameState &state)
{
    return state.lootNode >= 0 && static_cast<size_t>(state.lootNode) < state.gatherNodes.size() ?
        (IsOre(state.gatherNodes[state.lootNode].kind) ? 3.0f : 1.0f) : GameConstants::Loot::Duration;
}
inline int Nearby(const GameState &state)
{
    int closest = -1;
    float best = Range * Range;
    for (size_t index = 0; index < state.gatherNodes.size(); ++index)
    {
        const auto &node = state.gatherNodes[index];
        if (node.regrowTime > 0) continue;
        const float dx = node.position.x - state.player.x, dz = node.position.z - state.player.z;
        const float distance = dx*dx + dz*dz;
        if (distance < best) { best = distance; closest = static_cast<int>(index); }
    }
    return closest;
}
inline void Update(GameState &state, float dt)
{
    for (auto &node : state.gatherNodes) node.regrowTime = std::max(0.0f, node.regrowTime - dt);
}
inline std::wstring Start(GameState &state, int index)
{
    state.lootNode = index;
    state.lootAnimal = -1;
    state.lootTime = Duration(state);
    state.moving = false;
    const auto &node = state.gatherNodes[index];
    state.facing = {node.position.x-state.player.x, node.position.z-state.player.z};
    return std::wstring(ItemName(Item(node))) + (IsOre(node.kind) ? L" 채굴 중..." : L" 채집 중...");
}
inline std::wstring Complete(GameState &state)
{
    const int index = state.lootNode;
    state.lootNode = -1;
    if (index < 0 || static_cast<size_t>(index) >= state.gatherNodes.size()) return L"채집 대상이 없습니다.";
    auto &node = state.gatherNodes[index];
    const float dx=node.position.x-state.player.x, dz=node.position.z-state.player.z;
    if (node.regrowTime > 0 || dx*dx+dz*dz >= Range*Range) return L"채집 대상에서 너무 멀어졌습니다.";
    const int amount = IsOre(node.kind) ? 3 : 2;
    if (!state.inventory.Add(Item(node), amount)) return L"보관 공간이 부족합니다. 자원은 남아 있습니다.";
    node.regrowTime = IsOre(node.kind) ? 180.0f : 90.0f;
    state.lootPickupTime = GameConstants::Loot::PickupDuration;
    return std::wstring(ItemName(Item(node))) + L" " + std::to_wstring(amount) + L"개 획득!";
}
}
