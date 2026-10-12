#include "stdafx.h"
#include "GameUI.h"
#include "Renderer.h"

#include <algorithm>
#include <cwchar>
#include <vector>

void DrawGameUi(Renderer &renderer, const GameUiState &state)
{
    const float left = -state.windowWidth * .5f;
    const float top = state.windowHeight * .5f;
    const float dialogueWidth = std::min(620.0f, static_cast<float>(state.windowWidth - 40));
    const float healthRatio = std::max(0.0f, std::min(1.0f, state.health / std::max(1.0f, state.maxHealth)));
    const float experienceRatio = std::max(0.0f, std::min(1.0f, static_cast<float>(state.experience) /
                                                                  std::max(1, state.experienceToNextLevel)));

    // UI positions use pixels from the top-left; DrawLabel expects the lower-left of its texture.
    const auto label = [&](float x, float y, const wchar_t *text, float r, float g, float b) {
        renderer.DrawLabel(x, state.windowHeight - y - 32.0f, text, r, g, b);
    };
    const auto panel = [&](float x, float y, float width, float height) {
        renderer.DrawSolidRect(left + x + width * .5f, top - y - height * .5f, width, height,
                               0, .045f, .065f, .050f, .94f);
        renderer.DrawSolidRect(left + x + width * .5f, top - y - height * .5f, width - 4, height - 4,
                               0, .012f, .023f, .018f, .94f);
        renderer.DrawSolidRect(left + x + width * .5f, top - y - 2, width - 12, 2,
                               0, .56f, .44f, .23f, 1);
    };
    const auto wrap = [](const std::wstring &text, float width) {
        std::vector<std::wstring> lines;
        const size_t limit = static_cast<size_t>(std::max(1.0f, (width - 8) / 20.0f));
        size_t start = 0;
        while (start < text.size())
        {
            size_t end = std::min(start + limit, text.size());
            const size_t newline = text.find(L'\n', start);
            if (newline < end)
                end = newline;
            else if (end < text.size())
            {
                const size_t space = text.rfind(L' ', end);
                if (space != std::wstring::npos && space > start)
                    end = space;
            }
            lines.push_back(text.substr(start, end - start));
            start = end;
            while (start < text.size() && (text[start] == L' ' || text[start] == L'\n'))
                ++start;
        }
        return lines;
    };

    const float questWidth = std::min(320.0f, state.windowWidth * .5f - 30);
    const float questX = state.windowWidth - questWidth - 20;
    const auto objectiveLines = wrap(state.objective, questWidth - 28);
    panel(questX, 20, questWidth, 48 + objectiveLines.size() * 26.0f);
    label(questX + 14, 28, L"QUEST", .82f, .68f, .38f);
    for (size_t index = 0; index < objectiveLines.size(); ++index)
        label(questX + 14, 56 + index * 26.0f, objectiveLines[index].c_str(), .87f, .89f, .81f);

    panel(20, 20, 280, 148);
    renderer.DrawSolidRect(left + 160, top - 77, 252, 12, 0, .18f, .055f, .035f, 1);
    renderer.DrawSolidRect(left + 34 + healthRatio * 126, top - 77, healthRatio * 252, 8,
                           0, .66f, .18f, .12f, 1);
    renderer.DrawSolidRect(left + 160, top - 96, 252, 6, 0, .08f, .095f, .07f, 1);
    renderer.DrawSolidRect(left + 34 + experienceRatio * 126, top - 96, experienceRatio * 252, 4,
                           0, .70f, .55f, .25f, 1);
    wchar_t primaryStatus[80];
    wchar_t secondaryStatus[80];
    std::swprintf(primaryStatus, 80, L"LV.%d    HP %.0f / %.0f", state.level, state.health, state.maxHealth);
    std::swprintf(secondaryStatus, 80, L"ATK %d    EXP %d / %d", state.attackPower, state.experience,
                  state.experienceToNextLevel);
    label(32, 32, primaryStatus, .91f, .90f, .81f);
    label(32, 102, secondaryStatus, .71f, .77f, .69f);
    wchar_t inventory[80];
    std::swprintf(inventory, 80, L"P %d    H %d    T %d", state.potions, state.herbs, state.trophies);
    label(32, 130, inventory, .67f, .71f, .62f);
    if (!state.hint.empty())
    {
        const float hintWidth = 280;
        const float hintX = (state.windowWidth - hintWidth) * .5f;
        const auto hintLines = wrap(state.hint, hintWidth - 28);
        const float hintHeight = 16 + hintLines.size() * 26.0f;
        const float hintY = state.windowHeight - 20 - hintHeight;
        panel(hintX, hintY, hintWidth, hintHeight);
        for (size_t index = 0; index < hintLines.size(); ++index)
            label(hintX + 14, hintY + 6 + index * 26.0f, hintLines[index].c_str(), .92f, .78f, .46f);
    }
    if (state.messageTime > 0)
    {
        const auto messageLines = wrap(state.message, dialogueWidth - 32);
        const float dialogueHeight = 20 + messageLines.size() * 26.0f;
        const float dialogueX = (state.windowWidth - dialogueWidth) * .5f;
        const auto hintLines = wrap(state.hint, 252);
        const float hintSpace = state.hint.empty() ? 0 : 28 + hintLines.size() * 26.0f;
        const float dialogueY = state.windowHeight - 20 - hintSpace - dialogueHeight;
        panel(dialogueX, dialogueY, dialogueWidth, dialogueHeight);
        for (size_t index = 0; index < messageLines.size(); ++index)
            label(dialogueX + 16, dialogueY + 8 + index * 26.0f, messageLines[index].c_str(), .89f, .86f, .73f);
    }
}
