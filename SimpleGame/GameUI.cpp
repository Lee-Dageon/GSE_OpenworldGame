#include "stdafx.h"
#include "GameUI.h"
#include "Renderer.h"

#include <algorithm>
#include <cwchar>

void DrawGameUi(Renderer &renderer, const GameUiState &state)
{
    const float left = -state.windowWidth * .5f;
    const float top = state.windowHeight * .5f;
    const float dialogueWidth = std::min(560.0f, std::max(280.0f, static_cast<float>(state.windowWidth - 56)));
    const float healthRatio = std::max(0.0f, std::min(1.0f, state.health / state.maxHealth));
    const float experienceRatio = std::max(0.0f, std::min(1.0f, static_cast<float>(state.experience) /
                                                                  state.experienceToNextLevel));

    renderer.DrawSolidRect(left + 166, top - 42, 302, 58, 0, .008f, .016f, .013f, .88f);
    renderer.DrawSolidRect(left + 166, top - 14, 292, 2, 0, .78f, .56f, .16f, .92f);
    renderer.DrawLabel(20, static_cast<float>(state.windowHeight - 28), L"QUEST", 1.0f, .80f,
                       .36f);
    renderer.DrawLabel(78, static_cast<float>(state.windowHeight - 28), state.objective.c_str(), .94f, .96f, .88f);

    renderer.DrawSolidRect(left + 106, top - 82, 174, 66, 0, .008f, .016f, .013f, .88f);
    renderer.DrawSolidRect(left + 91, top - 71, 144, 14, 0, .20f, .035f, .028f, 1);
    renderer.DrawSolidRect(left + 20 + healthRatio * 72.0f, top - 71, healthRatio * 144.0f, 9, 0, .92f, .12f, .07f,
                           1);
    renderer.DrawSolidRect(left + 91, top - 91, 144, 10, 0, .01f, .015f, .014f, .86f);
    renderer.DrawSolidRect(left + 20 + experienceRatio * 72.0f, top - 91, experienceRatio * 144.0f, 5, 0, .95f, .67f,
                           .12f, 1);
    wchar_t primaryStatus[80];
    wchar_t secondaryStatus[80];
    std::swprintf(primaryStatus, 80, L"LV.%d  HP %.0f/%.0f  ATK %d", state.level, state.health, state.maxHealth,
                  state.attackPower);
    std::swprintf(secondaryStatus, 80, L"EXP %d/%d   P %d H %d T %d", state.experience, state.experienceToNextLevel,
                  state.potions, state.herbs, state.trophies);
    renderer.DrawLabel(20, static_cast<float>(state.windowHeight - 112), primaryStatus, .96f, .96f, .88f);
    renderer.DrawLabel(20, static_cast<float>(state.windowHeight - 136), secondaryStatus, .78f, .84f, .78f);
    if (!state.hint.empty())
    {
        renderer.DrawSolidRect(0, -state.windowHeight * .5f + 40, 270, 30, 0, .01f, .018f, .015f, .86f);
        renderer.DrawLabel(state.windowWidth * .5f - 112, 32, state.hint.c_str(), 1, .87f, .45f);
    }
    if (state.messageTime > 0)
    {
        renderer.DrawSolidRect(0, -top + 64, dialogueWidth, 42, 0, .012f, .02f, .018f, .90f);
        renderer.DrawSolidRect(0, -top + 84, dialogueWidth - 20, 2, 0, .72f, .52f, .15f, .75f);
        renderer.DrawLabel(std::max(20.f, state.windowWidth * .5f - dialogueWidth * .5f + 20), 48,
                           state.message.c_str(), .98f, .91f, .70f);
    }
}
