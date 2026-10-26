#include "stdafx.h"
#include "GameUI.h"
#include "Renderer.h"

#include <cmath>
#include <cwchar>
#include <vector>

void DrawGameUi(Renderer &renderer, const GameUiState &state)
{
    const float s = GameUiScale(state.windowWidth, state.windowHeight);
    const float left = -state.windowWidth * .5f, top = state.windowHeight * .5f;
    const auto rect = [&](float x, float y, float w, float h, float r, float g, float b, float a) {
        renderer.DrawSolidRect(left + x + w * .5f, top - y - h * .5f, w, h, 0, r, g, b, a);
    };
    const auto effect = [&](float x, float y, float radius, float r, float g, float b, float a, int type) {
        renderer.DrawEffectRect(left + x, top - y, radius * 2, radius * 2, 0, r, g, b, a, type);
    };
    const auto label = [&](float x, float y, const wchar_t *text, float size, float r, float g, float b) {
        // The cached font is 18 px. Do not shrink it below readable size on small windows.
        const float textScale = std::max(1.0f, size * 2.0f * s);
        const float baseline = state.windowHeight - y - 32 * textScale;
        for (const Vec2 offset : {Vec2{-1,0}, Vec2{1,0}, Vec2{0,-1}, Vec2{0,1}})
            renderer.DrawLabel(x + offset.x, baseline + offset.z, text, 0, 0, 0, textScale);
        renderer.DrawLabel(x, baseline, text, r, g, b, textScale);
    };
    const auto panel = [&](float x, float y, float w, float h, float alpha) {
        rect(x, y, w, h, .48f, .36f, .16f, alpha);
        rect(x + s, y + s, w - 2*s, h - 2*s, .025f, .030f, .032f, alpha);
        rect(x + 7*s, y + 2*s, w - 14*s, s, .78f, .64f, .33f, .72f);
    };
    const auto wrap = [](const std::wstring &text, size_t limit) {
        std::vector<std::wstring> result;
        for (size_t start = 0; start < text.size();)
        {
            size_t end = std::min(start + limit, text.size());
            const size_t newline = text.find(L'\n', start);
            if (newline < end) end = newline;
            else if (end < text.size())
            {
                const size_t space = text.rfind(L' ', end);
                if (space != std::wstring::npos && space > start) end = space;
            }
            result.push_back(text.substr(start, end-start));
            start = end;
            while (start < text.size() && (text[start] == L' ' || text[start] == L'\n')) ++start;
        }
        return result;
    };
    const float hp = std::max(0.0f, std::min(1.0f, state.health / std::max(1.0f, state.maxHealth)));
    const float xp = std::max(0.0f, std::min(1.0f, state.experience / static_cast<float>(std::max(1, state.experienceToNextLevel))));

    // Compact status sits directly over the world instead of a large opaque window.
    effect(34*s, 42*s, 19*s, .68f, .52f, .25f, .85f, 7);
    effect(34*s, 42*s, 16*s, .035f, .055f, .045f, .85f, 7);
    wchar_t text[100];
    std::swprintf(text, 100, L"%d", state.level);
    label(28*s, 31*s, text, .60f, 1, .86f, .51f);
    rect(62*s, 27*s, 164*s, 9*s, .09f, .025f, .02f, .80f);
    rect(64*s, 29*s, 160*s*hp, 5*s, .76f, .10f, .055f, .95f);
    std::swprintf(text, 100, L"HP %.0f / %.0f   공격력 %d", state.health, state.maxHealth, state.attackPower);
    label(63*s, 39*s, text, .42f, .91f, .88f, .76f);
    const float questWidth = std::min(340*s, state.windowWidth - 660*s);
    const float questX = (state.windowWidth - questWidth)*.5f;
    const float questLine = std::max(28.0f, 28*s);
    const size_t questLimit = static_cast<size_t>(std::max(10.0f, (questWidth-20*s)/(12*std::max(1.0f,1.04f*s))));
    const auto objective = wrap(state.objective, questLimit);
    panel(questX, 16*s, questWidth, 12*s + objective.size()*questLine, .80f);

    for (size_t i=0; i<objective.size(); ++i)
        label(questX+10*s, 20*s+i*questLine, objective[i].c_str(), .52f, 1, .97f, .87f);

    // Rotate world coordinates to match the quarter-view camera; UI y grows downward.
    const float mx = state.windowWidth - 93*s, my = 94*s, mr = 65*s;
    effect(mx, my, mr+2*s, .57f, .44f, .23f, 1, 7);
    const float mapScale = mr / 35.0f;
    const auto mapOffset = [&](Vec2 position) {
        const float x = position.x - state.player.x, z = position.z - state.player.z;
        return Vec2{(x-z)*.70710678f*mapScale, -(x+z)*.70710678f*mapScale};
    };
    const Vec2 lake = mapOffset(state.lake);
    // The shader uses upward-positive local coordinates, unlike the UI.
    effect(mx, my, mr, lake.x/mr, -lake.z/mr, state.lakeRadius/35.0f, 1, 8);
    const auto marker = [&](Vec2 position, float size, float r, float g, float b) {
        const Vec2 offset = mapOffset(position);
        const float dx = offset.x, dy = offset.z;
        if (dx*dx+dy*dy < (mr-size-3*s)*(mr-size-3*s))
        {
            effect(mx+dx, my+dy, size+s, .03f, .035f, .025f, 1, 7);
            effect(mx+dx, my+dy, size, r, g, b, 1, 7);
        }
    };
    marker(state.elder, 4*s, 1, .78f, .24f);
    if (state.monsterVisible) marker(state.monster, 6*s, .92f, .14f, .045f);
    if (state.hasObjectiveTarget) marker(state.objectiveTarget, 5*s, .90f, .74f, .27f);
    effect(mx, my, 5*s, .41f, .84f, 1, 1, 7);
    label(mx-(mr+16*s)*.70710678f-5*s, my-(mr+16*s)*.70710678f-12*s,
          L"N", .57f, .93f, .88f, .66f);
    label(mx-45*s, my+mr+7*s, L"그린웨이크", .56f, .88f, .87f, .76f);

    // Existing gameplay commands, with the same circles used by HitTestGameUi.
    const auto controls = GameUiControls(state.windowWidth, state.windowHeight);
    const wchar_t *keys[] = {L"LMB", L"Space", L"Q", L"E"};
    const wchar_t *names[] = {L"공격", L"회피", L"물약", L"상호작용"};
    for (size_t i=0; i<controls.size(); ++i)
    {
        const GameUiControl &control = controls[i];
        const bool enabled = i==2 ? state.potions>0 && hp<1 : (i==3 ? !state.hint.empty() : true);
        effect(control.x, control.y+3*s, control.radius+2*s, .015f, .017f, .02f, .7f, 7);
        effect(control.x, control.y, control.radius, enabled ? .77f : .30f, enabled ? .59f : .29f, enabled ? .28f : .26f, .98f, 7);
        effect(control.x, control.y, control.radius-3*s, .065f, .075f, .082f, .78f, 7);
        effect(control.x, control.y-control.radius*.25f, control.radius*.45f, i==2 ? .90f : 1.0f, i==2 ? .15f : .78f, i==2 ? .08f : .35f,
               enabled ? 1.0f : .4f, 10+static_cast<int>(i));
        const float progress = i==0 ? state.attackTime/.32f : (i==1 ? state.dodgeTime/.24f : 0);
        if (progress>0) effect(control.x, control.y, control.radius-3*s, std::min(1.0f, progress), 0, 0, .72f, 9);
        const float keyScale=std::max(1.0f,.84f*s);
        label(control.x-std::wcslen(keys[i])*5.2f*keyScale, control.y+control.radius*.30f,
              keys[i], .42f, 1, .96f, .81f);
        if (s >= .75f)
            label(control.x-std::wcslen(names[i])*9.0f*std::max(1.0f,.86f*s), control.y-control.radius-27*s,
                  names[i], .43f, .96f, .91f, .77f);
    }
    std::swprintf(text, 100, L"%d", state.potions);
    label(controls[2].x+16*s, controls[2].y-27*s, text, .50f, 1, .93f, .72f);


    std::swprintf(text, 100, L"약초 %d    전리품 %d    물약 %d", state.herbs, state.trophies, state.potions);
    label(30*s, state.windowHeight-76*s, text, .53f, .88f, .84f, .69f);
    label(25*s, state.windowHeight-35*s, L"WASD 이동   SHIFT 달리기", .44f, .70f, .73f, .66f);
    rect(0, state.windowHeight-6*s, static_cast<float>(state.windowWidth), 6*s, .035f, .03f, .025f, .9f);
    rect(0, state.windowHeight-5*s, state.windowWidth*xp, 3*s, .75f, .56f, .20f, 1);
    std::swprintf(text, 100, L"경험치 %d / %d", state.experience, state.experienceToNextLevel);
    label(state.windowWidth*.5f-55*s, state.windowHeight-26*s, text, .42f, .80f, .74f, .57f);

    const float toastWidth=std::min(440*s, state.windowWidth-620*s);
    const float toastX=(state.windowWidth-toastWidth)*.5f;
    const float toastLine=std::max(28.0f,28*s);
    const size_t toastLimit=static_cast<size_t>(std::max(10.0f,(toastWidth-20*s)/(12*std::max(1.0f,1.14f*s))));
    float toastY=state.windowHeight-std::max(38.0f,38*s);
    if (!state.hint.empty())
    {
        const auto lines=wrap(state.hint, toastLimit);
        const float h=14*s+lines.size()*toastLine;
        toastY-=h;
        panel(toastX, toastY, toastWidth, h, .82f);
        for (size_t i=0; i<lines.size(); ++i)
            label(toastX+10*s, toastY+5*s+i*toastLine, lines[i].c_str(), .57f, 1, .87f, .46f);
        toastY-=10*s;
    }
    if (state.messageTime>0 && !state.message.empty())
    {
        const auto lines=wrap(state.message, toastLimit);
        const float h=15*s+lines.size()*toastLine;
        toastY-=h;
        panel(toastX, toastY, toastWidth, h, std::min(.85f, state.messageTime));
        for (size_t i=0; i<lines.size(); ++i)
            label(toastX+10*s, toastY+6*s+i*toastLine, lines[i].c_str(), .57f, 1, .96f, .85f);
    }
}
