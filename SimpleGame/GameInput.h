#pragma once

#include "GameState.h"

enum class GameCommand
{
    None,
    Quit,
    Interact,
    UsePotion,
    Dodge,
    Attack,
    ToggleInventory,
    CraftHunterBlade,
    CraftHealingPotion,
    CraftHunterArmor
};

namespace GameInput
{
GameCommand OnKeyDown(GameState &state, unsigned char key, bool shiftHeld);
void OnKeyUp(GameState &state, unsigned char key);
GameCommand OnMouseButton(bool isLeftButton, bool isPressed);
void OnScroll(GameState &state, double verticalOffset);
} // namespace GameInput
