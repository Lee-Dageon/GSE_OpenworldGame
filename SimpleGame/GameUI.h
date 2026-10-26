#pragma once

#include <string>
#include <algorithm>
#include <array>
#include "GameInput.h"

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
    return std::max(.5f, std::min(1.4f, std::min(width / 1280.0f, height / 720.0f)));
}

inline std::array<GameUiControl, 4> GameUiControls(int width, int height)
{
    const float s = GameUiScale(width, height);
    return {{{width - 78 * s, height - 83 * s, 50 * s, GameCommand::Attack},
             {width - 180 * s, height - 57 * s, 34 * s, GameCommand::Dodge},
             {width - 56 * s, height - 191 * s, 31 * s, GameCommand::UsePotion},
             {width - 154 * s, height - 158 * s, 30 * s, GameCommand::Interact}}};
}

inline GameCommand HitTestGameUi(int x, int y, int width, int height)
{
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
