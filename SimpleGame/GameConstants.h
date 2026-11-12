#pragma once
#include <array>
#include <cstddef>

// Gameplay tuning. Times are seconds; sizes use the renderer's logical pixels.
// Map/monster/quest content stays in Data/*.gse; atlas coordinates stay with assets.
namespace GameConstants
{
namespace Loot
{
constexpr float HerbRegrowSeconds = 30.0f, HerbRange = 1.3f;
constexpr float Duration = 1.5f;
constexpr float SpriteSize = 90.34f;
constexpr int FrameCount = 4;
constexpr float FrameDuration = .5f; // Preserve the current animation cadence.
constexpr float PickupDuration = .6f;
}
namespace Player
{
constexpr float WalkSpeed = 2.4f, SprintSpeed = 4.0f, DodgeSpeed = 9.0f;
constexpr float DodgeDuration = .24f, PotionHealing = 45.0f;
constexpr float AttackDuration = .32f;
constexpr float WalkFps = 6.0f, SprintFps = 8.0f;
constexpr int DamageDivisor = 2, MinimumDamage = 1;
}
namespace Feedback
{
constexpr float HitStop = .06f, EffectDuration = .9f;
constexpr float Duration = HitStop + EffectDuration;
constexpr float FlashDuration = .16f, ShakeDuration = .25f, BorderDuration = .45f;
constexpr float MonsterFlashDuration = .22f;
constexpr float AttackHitStop = .05f, HeavyHitStop = .08f;
constexpr float AttackShakeDuration = .16f, AttackShakeStrength = .65f;
constexpr float ShakeX = 6.0f, ShakeY = 4.0f;
}
namespace Combo
{
constexpr float Window = 2.0f;
constexpr int HeavyInterval = 3, MaxExtraSparks = 3;
constexpr float TextScale = 3.0f, PopDuration = .22f, PopStrength = .4f, FadeDuration = .3f;
constexpr int TrailCount = 3;
constexpr float PopRise = 12, TrailX = 10, TrailY = 3, TrailAlpha = .15f;
constexpr float OutlineOffset = 3, ScreenY = .72f;
}
namespace HitEffect
{
constexpr float NumberDuration = .9f, Scale = 2.0f;
constexpr std::size_t MaxNumbers = 16;
constexpr float NumberScale = 1.2f, HeavyNumberScale = 1.5f, NumberFadeDuration = .2f;
constexpr float NumberRiseSpeed = 35, NumberHeight = 80, NumberSpread = 12;
constexpr float FramesPerSecond = 30, ExtraSparkDelay = .035f, SparkDuration = .5f;
constexpr float GoldFramesPerSecond = 36;
}
namespace Chapter1Boar
{
constexpr std::array<std::array<int, 6>, 3> FrameMilliseconds = {{
    {{120, 160, 100, 120, 120, 160}},
    {{40, 60, 100, 60, 60, 80}},
    {{120, 140, 160, 160, 180, 240}}
}};
}
namespace Inventory
{
constexpr int MaxStack = 99;
}
namespace World
{
constexpr int AnimalHealth = 12, AnimalExperience = 3;
constexpr float AnimalHitRange = 1.9f, AnimalLootRange = 1.5f, AnimalRespawnSeconds = 60.0f;
constexpr float Scale = 2.6f;
constexpr float DeerSpriteSize = 88.0f, BoarSpriteSize = 110.0f, SheepSpriteSize = 72.0f;
}
namespace Ui
{
constexpr float ReferenceWidth = 1280, ReferenceHeight = 720;
constexpr float MinimumScale = .5f, MaximumScale = 1.4f;
constexpr float InventoryWidth = 760, InventoryHeight = 540;
constexpr float MinimumFontScale = .55f;
}
namespace Timing
{
constexpr float MaxDeltaTime = .05f;
}
static_assert(Loot::Duration > 0 && Loot::FrameDuration > 0 && Loot::SpriteSize > 0, "Invalid loot tuning");
static_assert(Combo::HeavyInterval > 0 && Player::DamageDivisor > 0 && HitEffect::MaxNumbers > 0, "Invalid combat tuning");
}
