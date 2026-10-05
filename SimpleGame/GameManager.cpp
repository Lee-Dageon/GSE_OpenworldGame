#include "stdafx.h"
#include "GameManager.h"

#include "Monster.h"
#include "PlayerController.h"
#include "QuestSystem.h"

#include <algorithm>
#include <cmath>

GameManager::GameManager(GameState &state) : m_State(state) {}

void GameManager::BeginFrame(float deltaTime)
{
    m_State.time += deltaTime;
    m_State.dodge = std::max(0.0f, m_State.dodge - deltaTime);
    m_State.attack = std::max(0.0f, m_State.attack - deltaTime);
    m_State.messageTime = std::max(0.0f, m_State.messageTime - deltaTime);
    MonsterSystem::UpdateTimers(m_State.monster, deltaTime);
}

void GameManager::UpdateGame(float deltaTime)
{
    BeginFrame(deltaTime);
    PlayerController::UpdateMovement(m_State, deltaTime);
    for (AnimalState &animal : m_State.animals)
    {
        animal.phase += deltaTime * animal.speed;
        animal.position.x = animal.home.x + std::sin(animal.phase) * 2.4f;
        animal.position.z = animal.home.z + std::cos(animal.phase * .7f) * 1.8f;
    }
    if (m_State.quest == QuestStage::Hunt &&
        MonsterSystem::UpdateChaseAndAttack(m_State.monster, m_State.player, m_State.dodge, deltaTime, m_State.health))
        ShowMessage(L"The lake boar attacked you. Dodge with Space or heal with Q.");
    if (m_State.health <= 0.0f)
    {
        m_State.player = {0.0f, -2.5f};
        m_State.health = m_State.maxHealth;
        ShowMessage(L"The rescue party returned you to the village.");
    }
}

void GameManager::HandleCommand(GameCommand command)
{
    if (command == GameCommand::Interact)
    {
        const QuestSystem::InteractionResult result =
            QuestSystem::TryInteract(m_State, m_State.world.elder, m_State.world.tracks, m_State.world.herb);
        if (const wchar_t *message = QuestSystem::GetInteractionMessage(result))
            ShowMessage(message);
        return;
    }
    if (command == GameCommand::UsePotion && PlayerController::TryUsePotion(m_State))
    {
        ShowMessage(L"You used a potion.");
        return;
    }
    if (command == GameCommand::Dodge)
    {
        PlayerController::StartDodge(m_State);
        return;
    }
    if (command == GameCommand::Attack && m_State.attack <= 0.0f)
    {
        m_State.attack = .32f;
        if (m_State.quest != QuestStage::Hunt)
            return;
        const MonsterHitResult result = MonsterSystem::Hit(m_State.monster, m_State.player, m_State.attackPower);
        if (!result.defeated)
            return;
        m_State.quest = QuestStage::Trophy;
        ShowMessage(QuestSystem::AddExperience(m_State, 25) ? L"Level up! Your health and attack increased."
                                                          : L"You defeated the lake boar. Collect its trophy.");
    }
}

const wchar_t *GameManager::GetHint() const
{
    return QuestSystem::GetHint(m_State);
}

void GameManager::ShowMessage(const wchar_t *message)
{
    m_State.message = message;
    m_State.messageTime = 5.0f;
}

void GameManager::UpdateSceneGraph(Actor &sceneRoot, float deltaTime)
{
    sceneRoot.Update(deltaTime);
}
