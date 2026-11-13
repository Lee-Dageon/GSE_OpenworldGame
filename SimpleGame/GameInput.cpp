#include "stdafx.h"
#include "GameInput.h"
#include "BossIntro.h"

#include <cctype>
#include <algorithm>
#include <cmath>

namespace GameInput
{
GameCommand OnKeyDown(GameState &state, unsigned char key, bool shiftHeld)
{
    const unsigned char normalizedKey = static_cast<unsigned char>(std::tolower(key));
    const bool repeated = state.keys[normalizedKey];
    state.keys[normalizedKey] = true;
    state.sprint = shiftHeld;
    if (BossIntro::Active(state))
    {
        // Combat dodge presses must not dismiss an intro triggered while approaching the boss.
        if (!repeated && (normalizedKey == 27 || (state.bossDefeatIntro && normalizedKey == ' '))) BossIntro::Skip(state);
        return GameCommand::None;
    }

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
    case 'r':
        return repeated ? GameCommand::None : GameCommand::CraftHunterArmor;
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
void OnScroll(GameState &state, double verticalOffset)
{
    if (BossIntro::Active(state) || !std::isfinite(verticalOffset) || verticalOffset == 0) return;
    if (state.inventoryOpen)
    {
        const int lastPage = (static_cast<int>(ItemId::Count)-1) / GameConstants::Inventory::ItemsPerPage;
        state.inventoryPage = std::max(0, std::min(lastPage, state.inventoryPage + (verticalOffset > 0 ? -1 : 1)));
        return;
    }
    using namespace GameConstants::World;
    const double steps = std::max(-20.0, std::min(20.0, verticalOffset));
    state.cameraZoom = std::max(ZoomMinimum, std::min(ZoomMaximum,
        static_cast<float>(state.cameraZoom * std::pow(ZoomStep, steps))));
}
} // namespace GameInput
