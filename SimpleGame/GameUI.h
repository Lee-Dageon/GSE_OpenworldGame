#pragma once
#include "GameConstants.h"

#include <string>
#include <algorithm>
#include <array>
#include "GameInput.h"
#include "Inventory.h"

class Renderer;

struct GameUiState
{
    int windowWidth = 0;
    int windowHeight = 0;
    int level = 1;
    int attackPower = 0;
    int experience = 0;
    int experienceToNextLevel = 1;
    int potions = 0;
    int herbs = 0;
    int trophies = 0;
    int hides = 0;
    std::array<int, static_cast<size_t>(ItemId::Count)> itemCounts = {};
    bool inventoryOpen = false, hunterBladeCrafted = false, nearWorkbench = false, huntReported = false;
    bool armorRecipeUnlocked = false, hunterArmorCrafted = false;
    int defense = 0;
    std::wstring locationName = L"그린웨이크";
    float health = 0.0f;
    float maxHealth = 1.0f;
    float messageTime = 0.0f;
    float attackTime = 0.0f, dodgeTime = 0.0f;
    Vec2 player = {}, elder = {}, monster = {}, lake = {}, objectiveTarget = {};
    float lakeRadius = 0;
    bool monsterVisible = false, hasObjectiveTarget = false;
    std::wstring objective;
    std::wstring hint;
    std::wstring message;
};

struct GameUiControl
{
    float x, y, radius;
    GameCommand command;
};

inline float GameUiScale(int width, int height)
{
    return std::max(GameConstants::Ui::MinimumScale, std::min(GameConstants::Ui::MaximumScale, std::min(width / GameConstants::Ui::ReferenceWidth, height / GameConstants::Ui::ReferenceHeight)));
}

inline std::array<GameUiControl, 4> GameUiControls(int width, int height)
{
    const float s = GameUiScale(width, height);
    return {{{width - 78 * s, height - 83 * s, 50 * s, GameCommand::Attack},
             {width - 180 * s, height - 57 * s, 34 * s, GameCommand::Dodge},
             {width - 56 * s, height - 191 * s, 31 * s, GameCommand::UsePotion},
             {width - 154 * s, height - 158 * s, 30 * s, GameCommand::Interact}}};
}

struct GameUiButton
{
    float x, y, width, height;
    GameCommand command;
    bool Contains(int px, int py) const
    {
        return px >= x && px <= x+width && py >= y && py <= y+height;
    }
};

inline GameUiButton InventoryPanel(int width, int height)
{
    const float s = GameUiScale(width, height);
    return {(width-GameConstants::Ui::InventoryWidth*s)*.5f, (height-GameConstants::Ui::InventoryHeight*s)*.5f, GameConstants::Ui::InventoryWidth*s, GameConstants::Ui::InventoryHeight*s, GameCommand::None};
}

inline std::array<GameUiButton, 4> InventoryButtons(int width, int height)
{
    const float s = GameUiScale(width, height);
    const auto panel = InventoryPanel(width, height);
    return {{{panel.x+318*s, panel.y+174*s, 418*s, 78*s, GameCommand::CraftHunterBlade},
             {panel.x+318*s, panel.y+264*s, 418*s, 78*s, GameCommand::CraftHealingPotion},
             {panel.x+318*s, panel.y+354*s, 418*s, 78*s, GameCommand::CraftHunterArmor},
             {panel.x+panel.width-52*s, panel.y+12*s, 36*s, 32*s, GameCommand::ToggleInventory}}};
}

inline GameUiButton InventoryEntry(int width, int height)
{
    const float s = GameUiScale(width, height);
    return {18*s, height-118*s, 180*s, 30*s, GameCommand::ToggleInventory};
}

inline GameCommand HitTestGameUi(int x, int y, int width, int height, bool inventoryOpen = false)
{
    if (inventoryOpen)
    {
        for (const auto &button : InventoryButtons(width, height))
            if (button.Contains(x, y))
                return button.command;
        return GameCommand::None;
    }
    if (InventoryEntry(width, height).Contains(x, y))
        return GameCommand::ToggleInventory;
    for (const GameUiControl &control : GameUiControls(width, height))
        if ((x - control.x) * (x - control.x) + (y - control.y) * (y - control.y) <= control.radius * control.radius)
            return control.command;
    return GameCommand::None;
}

inline bool GameUiPanelContains(int x, int y, int width, int height)
{
    const float s = GameUiScale(width, height);
    const float questWidth = std::min(340*s, width-660*s);
    const float questLeft = (width-questWidth)*.5f;
    const float mapX = width-93*s, mapY = 94*s;
    return (x>=15*s && x<=230*s && y>=20*s && y<=63*s) ||
           (x>=questLeft && x<=questLeft+questWidth && y>=16*s && y<=65*s) ||
           ((x-mapX)*(x-mapX)+(y-mapY)*(y-mapY)<=70*s*70*s) ||
           (x>=18*s && x<=300*s && y>=height-81*s) ||
           (x>=315*s && x<=width-315*s && y>=height-185*s);
}

void DrawGameUi(Renderer &renderer, const GameUiState &state);
