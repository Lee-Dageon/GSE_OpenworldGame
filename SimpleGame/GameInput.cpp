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
    if (state.prologueTime>=0)
    {
        if (!repeated && normalizedKey==27)
        { state.prologueTime=-1; state.prologueSeen=true; state.player=state.world.spawn; state.moving=false; }
        return GameCommand::None;
    }
    if (BossIntro::Active(state))
    {
        // Combat dodge presses must not dismiss an intro triggered while approaching the boss.
        if (!repeated && (normalizedKey == 27 || (state.bossDefeatIntro && normalizedKey == ' '))) BossIntro::Skip(state);
        return GameCommand::None;
    }

    switch (normalizedKey)
    {
    case '[': return repeated ? GameCommand::None : GameCommand::SelectPrevious;
    case ']': return repeated ? GameCommand::None : GameCommand::SelectNext;
    case 'o': return repeated ? GameCommand::None : GameCommand::SortInventory;
    case 'x': return repeated ? GameCommand::None : GameCommand::DiscardItem;
    case 'b': return repeated ? GameCommand::None : GameCommand::DepositItem;
    case 'v': return repeated ? GameCommand::None : GameCommand::WithdrawItem;
    case 't': return repeated ? GameCommand::None : GameCommand::EquipWeapon;
    case 'y': return repeated ? GameCommand::None : GameCommand::EquipArmor;
    case 'f': return repeated ? GameCommand::None : GameCommand::CraftStyles;
    case 'z': return repeated ? GameCommand::None : GameCommand::SwitchStyle;
    case 'j': return repeated ? GameCommand::None : GameCommand::Journal;
    case 'g': return repeated ? GameCommand::None : GameCommand::BuildBase;
    case '1': return repeated ? GameCommand::None : GameCommand::ChooseFirst;
    case '2': return repeated ? GameCommand::None : GameCommand::ChooseSecond;
    case '3': return repeated ? GameCommand::None : GameCommand::ChooseThird;
    case '4': return repeated ? GameCommand::None : GameCommand::ChooseFourth;
    case '5': return repeated ? GameCommand::None : GameCommand::Rest;
    case 'k': return repeated ? GameCommand::None : GameCommand::SaveProgress;
    case 'l': return repeated ? GameCommand::None : GameCommand::LoadProgress;
    case 27:
        return repeated ? GameCommand::None : state.journalOpen ? GameCommand::Journal : (state.inventoryOpen ? GameCommand::ToggleInventory : GameCommand::Quit);
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
        state.inventorySelection=state.inventoryPage*GameConstants::Inventory::ItemsPerPage;
        return;
    }
    using namespace GameConstants::World;
    const double steps = std::max(-20.0, std::min(20.0, verticalOffset));
    state.cameraZoom = std::max(ZoomMinimum, std::min(ZoomMaximum,
        static_cast<float>(state.cameraZoom * std::pow(ZoomStep, steps))));
}
} // namespace GameInput
