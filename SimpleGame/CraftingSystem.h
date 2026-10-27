#pragma once
#include "GameState.h"

namespace CraftingSystem
{
enum class Recipe { HunterBlade, HealingPotion };
bool IsAtWorkbench(const GameState &state);
const wchar_t *TryCraft(GameState &state, Recipe recipe);
}
