#pragma once

#include "Actor.h"
#include "GameInput.h"
#include "GameState.h"

class GameManager
{
  public:
    explicit GameManager(GameState &state);

    void BeginFrame(float deltaTime);
    void UpdateGame(float deltaTime);
    void HandleCommand(GameCommand command);
    const wchar_t *GetHint() const;
    void UpdateSceneGraph(Actor &sceneRoot, float deltaTime);

  private:
    void ShowMessage(const wchar_t *message);
    void UpdateNpcs(float deltaTime);

    GameState &m_State;
};
