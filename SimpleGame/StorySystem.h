#pragma once
#include "GameContent.h"

namespace StorySystem
{
const GameContent::StoryStep *Current(const GameState &state);
bool CanStart(const GameState &state);
bool IsHunting(const GameState &state);
bool HasDrop(const GameState &state);
Vec2 Target(const GameState &state);
std::wstring Objective(const GameState &state);
const wchar_t *Hint(const GameState &state);
const wchar_t *RegionName(Vec2 position);
const wchar_t *Interact(GameState &state);
const wchar_t *EnemyDefeated(GameState &state);
void Update(GameState &state);
}
