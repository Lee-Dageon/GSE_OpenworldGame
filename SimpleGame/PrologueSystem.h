#pragma once
#include "InventoryActions.h"

namespace PrologueSystem
{
inline void Start(GameState &s)
{
    s.hunterBladeCrafted=s.hunterArmorCrafted=true; s.weaponTier=s.armorTier=1;
    InventoryActions::Equip(s,true,1); InventoryActions::Equip(s,false,1);
    s.prologueTime=0; s.prologueLossApplied=false;
    s.player={s.world.spawn.x-18,s.world.spawn.z-15};
}
inline void LoseEquipment(GameState &s)
{
    if (s.prologueLossApplied) return;
    s.lostWeaponTier=s.hunterBladeCrafted ? s.weaponTier : -1;
    s.lostArmorTier=s.hunterArmorCrafted ? s.armorTier : -1;
    InventoryActions::Equip(s,true,-1); InventoryActions::Equip(s,false,-1);
    s.hunterBladeCrafted=s.hunterArmorCrafted=s.armorRecipeUnlocked=false;
    s.weaponTier=s.armorTier=0; s.prologueLossApplied=true;
}
inline void Finish(GameState &s)
{
    LoseEquipment(s); s.prologueTime=-1; s.prologueSeen=true; s.player=s.world.spawn; s.moving=false;
    s.message=L"습격으로 철 장비를 잃었습니다. 미라 장로에게 E · 엔딩 뒤 J/9로 회수"; s.messageTime=7;
}
inline const wchar_t *Recover(GameState &s)
{
    if (!InventoryActions::AtStorage(s) || !s.storyFinished) return L"이야기 완료 후 장로 옆에서 잃은 장비를 회수하세요.";
    if (s.lostWeaponTier<0 && s.lostArmorTier<0) return L"회수할 분실 장비가 없습니다.";
    if (s.lostWeaponTier>=0)
    { s.hunterBladeCrafted=true; s.weaponTier=std::max(s.weaponTier,s.lostWeaponTier); InventoryActions::Equip(s,true,s.weaponTier); }
    if (s.lostArmorTier>=0)
    { s.hunterArmorCrafted=true; s.armorTier=std::max(s.armorTier,s.lostArmorTier); InventoryActions::Equip(s,false,s.armorTier); }
    s.lostWeaponTier=s.lostArmorTier=-1;
    return L"조사대가 분실 장비를 회수했습니다. 기존 강화 장비도 보존했습니다.";
}
}
