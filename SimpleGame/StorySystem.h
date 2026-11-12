#pragma once
#include "GameContent.h"

namespace StorySystem
{
const GameContent::StoryStep *Current(const GameState &state);
// Changes only the progress cursor; restore player/inventory/world separately.
bool SetProgressById(GameState &state, const std::string &chapterId, const std::string &stepId);
// Replaces gameplay state with a playable debug fixture; never use for save restoration.
bool DebugEnterStep(GameState &state, const std::string &stepId);
bool DebugEnterChapter(GameState &state, const std::string &chapterId);
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
