#pragma once
#include "GameConstants.h"
#include <array>
#include <cstddef>

enum class ItemId { Potion, Herb, BoarHide, BoarTusk, RawMeat, Wool, Antler, BeastBone, GuardianScale, AncientCrystal,
    WildBerry, Mushroom, Hardwood, PlantFiber, CopperOre, IronOre, SilverOre, GoldOre, Quartz, Sulfur, Coal, RockSalt,
    Amber, DesertRose, Moonflower, Sunstone, Count };

inline const wchar_t *ItemName(ItemId item)
{
    static const wchar_t *names[] = {L"회복 물약", L"약초", L"가죽", L"이빨", L"생고기",
        L"양털", L"사슴뿔", L"단단한 뼈", L"수호자 갑각", L"고대 결정",
        L"산딸기", L"버섯", L"단단한 목재", L"식물 섬유", L"구리 광석", L"철광석", L"은광석", L"금광석",
        L"석영", L"유황", L"석탄", L"암염", L"호박", L"사막 장미", L"달꽃", L"태양석"};
    const size_t index = static_cast<size_t>(item);
    return index < static_cast<size_t>(ItemId::Count) ? names[index] : L"알 수 없음";
}

inline const wchar_t *ItemTexture(ItemId item)
{
    static const wchar_t *paths[] = {
        L"./Assets/PixelArt/Items/raw-meat-v1.png", L"./Assets/PixelArt/Items/wool-v1.png",
        L"./Assets/PixelArt/Items/antler-v1.png", L"./Assets/PixelArt/Items/beast-bone-v1.png",
        L"./Assets/PixelArt/Items/guardian-scale-v1.png", L"./Assets/PixelArt/Items/ancient-crystal-v1.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-wild-berry.png", L"./Assets/PixelArt/Items/Frontier/frontier-mushroom.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-hardwood.png", L"./Assets/PixelArt/Items/Frontier/frontier-plant-fiber.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-copper-ore.png", L"./Assets/PixelArt/Items/Frontier/frontier-iron-ore.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-silver-ore.png", L"./Assets/PixelArt/Items/Frontier/frontier-gold-ore.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-quartz.png", L"./Assets/PixelArt/Items/Frontier/frontier-sulfur.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-coal.png", L"./Assets/PixelArt/Items/Frontier/frontier-rock-salt.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-amber.png", L"./Assets/PixelArt/Items/Frontier/frontier-desert-rose.png",
        L"./Assets/PixelArt/Items/Frontier/frontier-moonflower.png", L"./Assets/PixelArt/Items/Frontier/frontier-sunstone.png"};
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
