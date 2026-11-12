#pragma once
#include "GameState.h"
#include "CraftingSystem.h"
#include <algorithm>

namespace NpcSystem
{
inline int Role(const NpcState &npc)
{
    if (npc.identity==0) return 0;
    return npc.identity>0 ? 1+(npc.identity-1)%7 : 6;
}
inline const wchar_t *Name(const NpcState &npc)
{
    static const wchar_t *names[]={L"미라",L"도란",L"세라",L"로웬",L"가람",L"이안",L"보리",L"하린",
        L"타온",L"유나",L"루카",L"나루",L"에린",L"모아",L"레온",L"오린",L"리나",L"벤",L"소하",L"테오"};
    return npc.identity>=0 && npc.identity<20 ? names[npc.identity] : L"여행자";
}
inline const wchar_t *Job(const NpcState &npc)
{
    static const wchar_t *jobs[]={L"장로",L"대장장이",L"약사",L"교역상",L"사냥꾼",L"조사원",L"농부",L"정찰병"};
    return jobs[Role(npc)];
}
inline const wchar_t *Hint(const NpcState &npc)
{
    static const wchar_t *hints[]={L"E 의뢰·보고",L"E 검 제작 · I 장비",L"E 약초 1 → 물약 1",L"E 가죽 2 → 물약 1",
        L"E 사냥 조언",L"E 유적 이야기",L"E 약초 1 → 식사·회복",L"E 길 안내"};
    return hints[Role(npc)];
}
inline int Nearby(const GameState &state)
{
    // Preserve the elder's quest interaction, including custom maps.
    const float ex=state.player.x-state.world.elder.x,ez=state.player.z-state.world.elder.z;
    if (ex*ex+ez*ez<1.8f*1.8f) return -1;
    int result=-1;float best=GameConstants::Npc::InteractRange*GameConstants::Npc::InteractRange;
    for (size_t i=0;i<state.npcs.size();++i)
    {
        const auto &npc=state.npcs[i];
        const float x=npc.position.x-state.player.x,z=npc.position.z-state.player.z;
        if (Role(npc)!=0 && x*x+z*z<best) {best=x*x+z*z;result=static_cast<int>(i);}
    }
    return result;
}
inline std::wstring Interact(GameState &state, NpcState &npc)
{
    const int role=Role(npc);
    const bool alternate=(npc.conversations%2)!=0;
    npc.conversations=(npc.conversations+1)%2;
    const wchar_t *reply=L"오늘도 무사히 돌아오세요.";
    if (role==1) reply=CraftingSystem::TryCraft(state,CraftingSystem::Recipe::HunterBlade);
    else if (role==2) reply=CraftingSystem::TryCraft(state,CraftingSystem::Recipe::HealingPotion);
    else if (role==3)
    {
        if (state.inventory.Count(ItemId::BoarHide)<GameConstants::Npc::TradeHideCost) reply=L"가죽 2개를 가져오면 물약 1개로 바꿔 드리죠.";
        else if (!state.inventory.Add(ItemId::Potion)) reply=L"물약 가방이 가득 찼군요. 가죽은 그대로 두세요.";
        else {state.inventory.Remove(ItemId::BoarHide,GameConstants::Npc::TradeHideCost);reply=L"가죽 2개를 받았습니다. 회복 물약 1개입니다.";}
    }
    else if (role==4) reply=state.ruinDeviceDisabled ? L"차타카브라가 양팔을 내리찍은 뒤 혀를 노리세요. 돌을 깨면 빈틈이 생깁니다." :
        alternate ? L"큰 공격은 옆으로 피하고 빈틈에 한 번씩 치세요." : L"명중을 이어 가면 콤보가 쌓입니다. 욕심내다 맞으면 끊기죠.";
    else if (role==5) reply=state.ruinDeviceDisabled ? L"장치가 멎었군요. 생태계가 회복하는지는 계속 지켜봐야 합니다." :
        state.armorRecipeUnlocked ? L"짐승들이 같은 방향을 피해 이동합니다. 북쪽 유적의 기록을 찾아보세요." :
        alternate ? L"조사대 기록이 남아 있다면 이동 원인을 찾을 수 있을 겁니다." : L"짐승이 마을로 온 이유부터 알아야 합니다. 흔적을 남겨 주세요.";
    else if (role==6)
    {
        if (state.health>=state.maxHealth) reply=alternate ? L"동물에게서 얻은 가죽은 교역상에게 가져가 보세요." : L"배가 든든해 보이네요. 약초는 다시 자란답니다.";
        else if (state.inventory.Count(ItemId::Herb)<1) reply=L"약초 1개를 가져오면 따뜻한 식사로 체력 25를 회복해 드릴게요.";
        else {state.inventory.Remove(ItemId::Herb);state.health=std::min(state.maxHealth,state.health+GameConstants::Npc::MealHealing);reply=L"따뜻할 때 드세요. 체력 25 회복!";}
    }
    else if (role==7) reply=alternate ? L"숲에서 무리하지 마세요. 물약과 장비를 챙긴 뒤 출발하세요." : L"남쪽 길목 너머는 위험합니다. 북부 숲과 폐허에서는 흔적을 살피세요.";
    return std::wstring(Name(npc))+L" · "+Job(npc)+L": "+reply;
}
}
