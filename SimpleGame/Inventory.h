#pragma once
#include "GameConstants.h"
#include <array>
#include <cstddef>

enum class ItemId { Potion, Herb, BoarHide, BoarTusk, RawMeat, Wool, Antler, BeastBone, GuardianScale, AncientCrystal, Count };

inline const wchar_t *ItemName(ItemId item)
{
    static const wchar_t *names[] = {L"회복 물약", L"약초", L"가죽", L"이빨", L"생고기",
        L"양털", L"사슴뿔", L"단단한 뼈", L"수호자 갑각", L"고대 결정"};
    const size_t index = static_cast<size_t>(item);
    return index < static_cast<size_t>(ItemId::Count) ? names[index] : L"알 수 없음";
}

inline const wchar_t *ItemTexture(ItemId item)
{
    static const wchar_t *paths[] = {
        L"./Assets/PixelArt/Items/raw-meat-v1.png", L"./Assets/PixelArt/Items/wool-v1.png",
        L"./Assets/PixelArt/Items/antler-v1.png", L"./Assets/PixelArt/Items/beast-bone-v1.png",
        L"./Assets/PixelArt/Items/guardian-scale-v1.png", L"./Assets/PixelArt/Items/ancient-crystal-v1.png"};
    const size_t index = static_cast<size_t>(item);
    return index >= 4 && index < static_cast<size_t>(ItemId::Count) ? paths[index - 4] : nullptr;
}

struct Inventory
{
    static constexpr int MaxStack = GameConstants::Inventory::MaxStack;
    std::array<int, static_cast<size_t>(ItemId::Count)> stacks = {{2, 0, 0, 0}};

    int Count(ItemId item) const
    {
        const size_t index = static_cast<size_t>(item);
        return index < stacks.size() ? stacks[index] : 0;
    }
    bool Add(ItemId item, int amount = 1)
    {
        const size_t index = static_cast<size_t>(item);
        if (index >= stacks.size() || amount <= 0 || amount > MaxStack - stacks[index])
            return false;
        stacks[index] += amount;
        return true;
    }
    bool Remove(ItemId item, int amount = 1)
    {
        const size_t index = static_cast<size_t>(item);
        if (index >= stacks.size() || amount <= 0 || stacks[index] < amount)
            return false;
        stacks[index] -= amount;
        return true;
    }
};
