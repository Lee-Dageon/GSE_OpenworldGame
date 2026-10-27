#pragma once
#include <array>
#include <cstddef>

enum class ItemId { Potion, Herb, BoarHide, BoarTusk, Count };

struct Inventory
{
    static constexpr int MaxStack = 99;
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
