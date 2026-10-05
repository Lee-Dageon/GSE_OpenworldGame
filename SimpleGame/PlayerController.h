#pragma once

#include "GameState.h"

namespace PlayerController
{
void UpdateMovement(GameState &state, float deltaTime);
bool TryUsePotion(GameState &state);
void StartDodge(GameState &state);
} // namespace PlayerController
