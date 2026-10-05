#pragma once

#include "GameState.h"

enum class GameCommand
{
    None,
    Quit,
    Interact,
    UsePotion,
    Dodge,
    Attack
};

namespace GameInput
{
GameCommand OnKeyDown(GameState &state, unsigned char key, bool shiftHeld);
void OnKeyUp(GameState &state, unsigned char key);
GameCommand OnMouseButton(bool isLeftButton, bool isPressed);
} // namespace GameInput
