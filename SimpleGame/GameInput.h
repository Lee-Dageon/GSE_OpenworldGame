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
    CraftHunterArmor,
    ChooseFirst, ChooseSecond, SaveProgress, LoadProgress,
    SelectPrevious, SelectNext, SortInventory, DiscardItem, DepositItem, WithdrawItem,
    EquipWeapon, EquipArmor, CraftStyles, SwitchStyle, Journal, BuildBase,
    ChooseThird, ChooseFourth, Rest, ChooseSixth, ChooseSeventh, ChooseEighth, ChooseNinth,
    CallHorse
};

namespace GameInput
{
GameCommand OnKeyDown(GameState &state, unsigned char key, bool shiftHeld);
void OnKeyUp(GameState &state, unsigned char key);
GameCommand OnMouseButton(bool isLeftButton, bool isPressed);
void OnScroll(GameState &state, double verticalOffset);
} // namespace GameInput
