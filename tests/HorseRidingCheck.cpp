#include "../SimpleGame/HorseSystem.h"
#include "../SimpleGame/PlayerController.h"
#include "../SimpleGame/GameManager.h"
#include "../SimpleGame/SaveGame.h"
#include "../SimpleGame/HorseSpriteFrames.h"
#include <cassert>
#include <iostream>

int main()
{
    GameState state = CreateInitialGameState();
    state.player = {-3, -1.7f};
    state.horse.visible = true;
    state.horse.position = {-9, -1.7f};
    HorseSystem::Call(state);
    assert(state.horse.called && state.horse.route.points.size() > 1); // House requires a detour.
    bool mounted = false;
    for (int i = 0; i < 200 && !mounted; ++i)
    {
        const Vec2 before = state.horse.position;
        mounted = HorseSystem::Update(state, .02f);
        assert(WorldLayout::CanTravel(state.world, before, state.horse.position, .12f));
        // Mounting snaps the horse the final MountRange; the player never teleports.
        assert(Navigation::Distance(before, state.horse.position) <= .401f + GameConstants::Horse::MountRange);
        assert(state.player.x == -3 && state.player.z == -1.7f);
    }
    assert(mounted && state.horse.mounted && !state.horse.called);
    HorseSystem::Dismount(state);
    assert(!state.horse.mounted && state.horse.visible && !state.horse.called);

    // Compare real controller displacement on a collision-free corridor.
    bool corridor = false;
    for (int x = -40; x <= -20 && !corridor; ++x)
        for (int z = -40; z <= -20; ++z)
            if (WorldLayout::CanTravel(state.world, {float(x),float(z)}, {x+2.0f,z+2.0f}))
            { state.player = {float(x),float(z)}; corridor = true; break; }
    assert(corridor);
    state.keys['w'] = true;
    state.sprint = true;
    const Vec2 origin = state.player;
    PlayerController::UpdateMovement(state, .1f);
    const float sprintDistance = Navigation::Distance(origin, state.player);
    state.player = origin;
    state.horse.mounted = true;
    state.sprint = false; // Riding does not require holding Shift.
    PlayerController::UpdateMovement(state, .1f);
    assert(std::abs(Navigation::Distance(origin, state.player) - 5*sprintDistance) < .001f);
    assert(!PlayerController::StartDodge(state)); // Manager must dismount before a roll.

    state.player = {state.world.lake.x + state.world.lakeRadius + 2, state.world.lake.z};
    state.keys.fill(false); state.keys['s'] = state.keys['a'] = true;
    PlayerController::UpdateMovement(state, 1);
    assert(state.player.x >= state.world.lake.x + state.world.lakeRadius);
    state.player = origin;

    state.keys.fill(false);
    state.attack = state.dodge = state.hitStop = 0;
    GameManager manager(state);
    manager.HandleCommand(GameCommand::Attack);
    assert(!state.horse.mounted && state.attack > 0);
    state.attack = 0; state.attackPending = false; state.horse.mounted = true;
    manager.HandleCommand(GameCommand::Dodge);
    assert(!state.horse.mounted && state.dodge > 0);
    state.dodge = 0; state.horse.mounted = true;
    manager.HandleCommand(GameCommand::CallHorse);
    assert(!state.horse.mounted);
    assert(GameInput::OnKeyDown(state, 'H', false) == GameCommand::CallHorse);
    assert(GameInput::OnKeyDown(state, 'h', false) == GameCommand::None);
    GameInput::OnKeyUp(state, 'h');

    state.horse.called = true;
    state.inventoryOpen = true;
    const Vec2 parked = state.horse.position;
    manager.UpdateGame(1);
    assert(Navigation::Distance(parked, state.horse.position) == 0 && !state.horse.mounted);
    state.inventoryOpen = false;
    state.world.minimumCoordinate = state.world.maximumCoordinate = 0;
    state.horse = {};
    HorseSystem::Call(state);
    assert(!state.horse.visible && !state.horse.called); // No reachable spawn.

    auto saved = CreateInitialGameState();
    saved.horse.visible = saved.horse.mounted = true;
    GameState loaded;
    assert(SaveGame::Decode(SaveGame::Encode(saved), saved, loaded));
    assert(!loaded.horse.visible && !loaded.horse.mounted); // Transient; old save format stays intact.
    assert(HorseSpriteFrames::Direction(1,1) == 5);
    assert(HorseSpriteFrames::Direction(-1,-1) == 1);
    std::cout << "PASS: horse detour, arrival, 5x speed, dismount, commands, pause, failed call, save compatibility\n";
}
