#include "stdafx.h"
#include "GameConstants.h"
#include "GameUI.h"
#include "Renderer.h"

#include <cmath>
#include <cwchar>
#include <vector>

static void DrawInventoryPanel(Renderer &renderer, const GameUiState &state);

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
    std::swprintf(text, 100, L"체력 %.0f / %.0f   공격력 %d", state.health, state.maxHealth, state.attackPower);
    label(63*s, 39*s, text, .42f, .91f, .88f, .76f);
    const float questWidth = std::min(340*s, state.windowWidth - 660*s);
    const float questX = (state.windowWidth - questWidth)*.5f;
    const float questLine = std::max(28.0f, 28*s);
    const size_t questLimit = static_cast<size_t>(std::max(6.0f, (questWidth-20*s)/(18*std::max(1.0f,1.04f*s))));
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
    const auto marker = [&](Vec2 position, float size, float r, float g, float b, bool clampToEdge) {
        const Vec2 offset = mapOffset(position);
        float dx = offset.x, dy = offset.z;
        const float limit = mr-size-3*s, distance = std::sqrt(dx*dx+dy*dy);
        if (distance >= limit && clampToEdge && distance > 0)
        {
            dx *= limit/distance; dy *= limit/distance;
        }
        if (clampToEdge || distance < limit)
        {
            effect(mx+dx, my+dy, size+s, .03f, .035f, .025f, 1, 7);
            effect(mx+dx, my+dy, size, r, g, b, 1, 7);
        }
    };
    marker(state.elder, 4*s, 1, .78f, .24f, false);
    if (state.monsterVisible) marker(state.monster, 6*s, .92f, .14f, .045f, false);
    if (state.hasObjectiveTarget) marker(state.objectiveTarget, 5*s, .90f, .74f, .27f, true);
    effect(mx, my, 5*s, .41f, .84f, 1, 1, 7);
    label(mx-(mr+16*s)*.70710678f-5*s, my-(mr+16*s)*.70710678f-12*s,
          L"북", .57f, .93f, .88f, .66f);
    label(mx-45*s, my+mr+7*s, state.locationName.c_str(), .56f, .88f, .87f, .76f);

    // Existing gameplay commands, with the same circles used by HitTestGameUi.
    const auto controls = GameUiControls(state.windowWidth, state.windowHeight);
    const wchar_t *keys[] = {L"좌클릭", L"공백", L"Q", L"E"};
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
        label(control.x-std::wcslen(keys[i])*(i<2 ? 9.0f : 5.2f)*keyScale, control.y+control.radius*.30f,
              keys[i], .42f, 1, .96f, .81f);
        if (s >= .75f)
            label(control.x-std::wcslen(names[i])*9.0f*std::max(1.0f,.86f*s), control.y-control.radius-27*s,
                  names[i], .43f, .96f, .91f, .77f);
    }
    std::swprintf(text, 100, L"%d", state.potions);
    label(controls[2].x+16*s, controls[2].y-27*s, text, .50f, 1, .93f, .72f);


    std::swprintf(text, 100, L"약초 %d    전리품 %d    물약 %d", state.herbs, state.trophies, state.potions);
    label(30*s, state.windowHeight-76*s, text, .53f, .88f, .84f, .69f);
    label(25*s, state.windowHeight-35*s, L"WASD 이동   시프트 달리기", .44f, .70f, .73f, .66f);
    rect(0, state.windowHeight-6*s, static_cast<float>(state.windowWidth), 6*s, .035f, .03f, .025f, .9f);
    rect(0, state.windowHeight-5*s, state.windowWidth*xp, 3*s, .75f, .56f, .20f, 1);
    std::swprintf(text, 100, L"경험치 %d / %d", state.experience, state.experienceToNextLevel);
    label(state.windowWidth*.5f-55*s, state.windowHeight-26*s, text, .42f, .80f, .74f, .57f);

    const float toastWidth=std::min(440*s, state.windowWidth-620*s);
    const float toastX=(state.windowWidth-toastWidth)*.5f;
    const float toastLine=std::max(28.0f,28*s);
    const size_t toastLimit=static_cast<size_t>(std::max(6.0f,(toastWidth-20*s)/(18*std::max(1.0f,1.14f*s))));
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
    const auto entry = InventoryEntry(state.windowWidth, state.windowHeight);
    panel(entry.x, entry.y, entry.width, entry.height, .88f);
    label(entry.x+9*s, entry.y+2*s, L"I  가방 / 제작", .46f, 1, .88f, .60f);
    if (state.inventoryOpen)
        DrawInventoryPanel(renderer, state);
}

static void DrawInventoryPanel(Renderer &renderer, const GameUiState &state)
{
    const float s = GameUiScale(state.windowWidth, state.windowHeight);
    const auto panel = InventoryPanel(state.windowWidth, state.windowHeight);
    const auto rect = [&](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
        renderer.DrawSolidRect(panel.x+(x+w*.5f)*s-state.windowWidth*.5f,
            state.windowHeight*.5f-panel.y-(y+h*.5f)*s, w*s, h*s, 0, r,g,b,a);
    };
    const auto text = [&](float x, float y, const wchar_t *value, float size = .85f,
                          float r = .88f, float g = .90f, float b = .86f) {
        const float textScale = std::max(GameConstants::Ui::MinimumFontScale, size*s);
        renderer.DrawLabel(panel.x+x*s, state.windowHeight-panel.y-y*s-28*textScale,
            value, r,g,b, textScale);
    };
    const GLuint icons = renderer.UiTexture(L"./Assets/PixelArt/Items/inventory-icons-v1.png");
    const GLuint loot = renderer.UiTexture(L"./Assets/PixelArt/Items/boar-loot-v1.png");
    const auto icon = [&](float x, float y, float size, int item) {
        const bool material = item >= 4;
        const GLuint texture = material ? loot : icons;
        if (texture == 0) { text(x+8,y+8,L"◇",1.4f, .8f,.65f,.35f); return; }
        renderer.DrawSprite(texture, panel.x+(x+size*.5f)*s-state.windowWidth*.5f,
            state.windowHeight*.5f-panel.y-(y+size*.5f)*s, size*s,size*s,
            material ? item-4 : item%2, material ? 0 : item/2, 2, material ? 1 : 2,
            1,1,1,1,0,0,0,1,1,.5f);
    };
    renderer.DrawSolidRect(0,0,static_cast<float>(state.windowWidth),static_cast<float>(state.windowHeight),
        0, .005f,.008f,.012f,.78f);
    rect(0,0,GameConstants::Ui::InventoryWidth,GameConstants::Ui::InventoryHeight,.42f,.34f,.20f);
    rect(2,2,GameConstants::Ui::InventoryWidth-4,GameConstants::Ui::InventoryHeight-4,.035f,.05f,.065f);
    rect(2,2,GameConstants::Ui::InventoryWidth-4,54,.065f,.085f,.10f);
    text(24,12,L"가방",1.2f,1,.88f,.59f);
    text(110,20,L"재료 · 장비 · 제작",.7f,.57f,.65f,.67f);
    rect(24,70,712,66,.065f,.09f,.105f);
    text(38,78,state.hunterBladeCrafted ? L"이빨 검" : L"기본 검",.9f);
    text(190,78,state.hunterArmorCrafted ? L"가죽 방어구" : L"기본 의상",.9f);
    text(38,109,L"장착 장비",.55f,.52f,.63f,.63f);
    wchar_t line[120];
    std::swprintf(line,120,L"피해 %d   방어 %d",std::max(GameConstants::Player::MinimumDamage,state.attackPower/GameConstants::Player::DamageDivisor),state.defense);
    text(490,88,line,.95f,1,.84f,.48f);
    text(24,148,L"보유 재료",.8f,1,.87f,.58f);
    text(318,148,L"야영지 제작",.8f,1,.87f,.58f);
    const wchar_t *names[] = {L"회복 물약",L"약초",L"가죽",L"이빨"};
    const int counts[] = {state.potions,state.herbs,state.hides,state.trophies};
    const int itemIcons[] = {0,1,5,4};
    for (int item=0;item<4;++item)
    {
        const float x=24+(item%2)*136.0f, y=174+(item/2)*134.0f;
        rect(x,y,124,122,.14f,.20f,.23f);
        rect(x+1,y+1,122,120,.055f,.085f,.105f);
        icon(x+28,y+9,68,itemIcons[item]);
        text(x+10,y+82,names[item],.8f);
        std::swprintf(line,120,L"%d",counts[item]);
        text(x+86,y+9,line,.95f,1,.87f,.5f);
        rect(x+10,y+110,104,3,.025f,.04f,.05f);
        rect(x+10,y+110,104*std::min(1.0f,counts[item]/static_cast<float>(GameConstants::Inventory::MaxStack)),3,.48f,.65f,.38f);
    }
    const std::wstring stackLimit = L"아이템별 최대 " + std::to_wstring(GameConstants::Inventory::MaxStack) + L"개";
    text(24,449,stackLimit.c_str(),.6f,.52f,.63f,.65f);
    const auto buttons=InventoryButtons(state.windowWidth,state.windowHeight);
    const wchar_t *titles[]={L"이빨 검",L"회복 물약",L"가죽 방어구"};
    const wchar_t *costs[]={L"가죽 1 + 이빨 1",L"약초 1",L"가죽 2 + 이빨 1"};
    const wchar_t *keys[]={L"C",L"P",L"R"};
    const int recipeIcons[]={2,0,3};
    for (int recipe=0;recipe<3;++recipe)
    {
        const auto &button=buttons[recipe];
        const float x=(button.x-panel.x)/s, y=(button.y-panel.y)/s;
        const bool equipped=recipe==0 ? state.hunterBladeCrafted : (recipe==2 && state.hunterArmorCrafted);
        const bool unlocked=recipe==0 ? state.huntReported : (recipe==1 || state.armorRecipeUnlocked);
        const bool enough=recipe==0 ? state.hides>=1 && state.trophies>=1 :
            (recipe==1 ? state.herbs>=1 : state.hides>=2 && state.trophies>=1);
        const bool full=recipe==1 && state.potions>=GameConstants::Inventory::MaxStack;
        const bool enabled=state.nearWorkbench && !equipped && unlocked && enough && !full;
        rect(x,y,418,78,enabled ? .46f : .16f,enabled ? .48f : .22f,enabled ? .27f : .25f);
        rect(x+1,y+1,416,76,enabled ? .10f : .05f,enabled ? .17f : .075f,.09f);
        icon(x+9,y+9,60,recipeIcons[recipe]);
        text(x+82,y+8,titles[recipe],.95f,enabled ? 1 : .72f,.88f,.65f);
        text(x+82,y+42,costs[recipe],.65f,.61f,.72f,.7f);
        const wchar_t *status=equipped ? L"장착 중" : (!unlocked ? L"의뢰 잠김" :
            (!state.nearWorkbench ? L"마을 제작" : (full ? L"보관 가득" : (!enough ? L"재료 부족" : L"제작"))));
        text(x+324,y+11,status,.65f,enabled ? .85f : .53f,enabled ? 1 : .61f,.63f);
        text(x+350,y+43,keys[recipe],.75f,.72f,.8f,.72f);
    }
    rect(24,475,712,1,.18f,.24f,.25f);
    std::wstring status=state.messageTime>0 ? state.message :
        (state.nearWorkbench ? L"제작하면 자동 장착됩니다." : L"장로 근처에서 제작할 수 있습니다.");
    const size_t statusLimit = static_cast<size_t>(712*s/(18*std::max(GameConstants::Ui::MinimumFontScale,.65f*s)));
    if (status.size()>statusLimit) status=status.substr(0,statusLimit-1)+L"…";
    text(24,486,status.c_str(),.65f,.74f,.79f,.72f);
    text(24,516,L"I / ESC  닫기",.55f,.48f,.58f,.60f);
    text(614,516,L"일시정지",.55f,.70f,.65f,.47f);
    const auto &close=buttons[3];
    rect((close.x-panel.x)/s,(close.y-panel.y)/s,close.width/s,close.height/s,.16f,.21f,.22f);
    text((close.x-panel.x)/s+9,(close.y-panel.y)/s+1,L"×",1.0f);
}
