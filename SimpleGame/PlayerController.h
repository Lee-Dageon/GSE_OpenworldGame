#pragma once

#include "GameState.h"

namespace PlayerController
{
void UpdateMovement(GameState &state, float deltaTime, float dodgeAtFrameStart=-1.0f);
bool TryUsePotion(GameState &state);
bool StartDodge(GameState &state);
} // namespace PlayerController
