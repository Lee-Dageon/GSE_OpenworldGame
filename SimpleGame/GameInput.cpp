#include "stdafx.h"
#include "GameInput.h"

#include <cctype>

namespace GameInput
{
GameCommand OnKeyDown(GameState &state, unsigned char key, bool shiftHeld)
{
    const unsigned char normalizedKey = static_cast<unsigned char>(std::tolower(key));
    const bool repeated = state.keys[normalizedKey];
    state.keys[normalizedKey] = true;
    state.sprint = shiftHeld;

    switch (normalizedKey)
    {
    case 27:
        return repeated ? GameCommand::None : (state.inventoryOpen ? GameCommand::ToggleInventory : GameCommand::Quit);
    case 'i':
        return repeated ? GameCommand::None : GameCommand::ToggleInventory;
    case 'c':
        return repeated ? GameCommand::None : GameCommand::CraftHunterBlade;
    case 'p':
        return repeated ? GameCommand::None : GameCommand::CraftHealingPotion;
    case 'e':
        return GameCommand::Interact;
    case 'q':
        return GameCommand::UsePotion;
    case ' ':
        return GameCommand::Dodge;
    default:
        return GameCommand::None;
    }
}

void OnKeyUp(GameState &state, unsigned char key)
{
    state.keys[static_cast<unsigned char>(std::tolower(key))] = false;
    state.sprint = false;
}

GameCommand OnMouseButton(bool isLeftButton, bool isPressed)
{
    return isLeftButton && isPressed ? GameCommand::Attack : GameCommand::None;
}
} // namespace GameInput
