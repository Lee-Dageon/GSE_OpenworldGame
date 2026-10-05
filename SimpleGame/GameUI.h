#pragma once

#include <string>

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
    std::wstring objective;
    std::wstring hint;
    std::wstring message;
};

void DrawGameUi(Renderer &renderer, const GameUiState &state);
