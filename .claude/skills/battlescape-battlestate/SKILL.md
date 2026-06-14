---
name: battlescape-battlestate
description: >-
  Work with Battlescape BattleStates (the *BState classes) — the tactical action sub-machine
  that sequences a single battle action: walking, turning, projectile flight, explosions, melee,
  psi, unit death/fall/panic. Use when adding or modifying a *BState, changing how a battle
  action plays out step-by-step, sequencing follow-up actions (e.g. shot -> explosion -> death),
  reasoning about the BattlescapeGame state queue (statePushBack/Front/Next, popState, handleState),
  TU spending, or the end-turn sentinel. This is a SEPARATE machine from the engine State stack
  (Game::pushState) — do not confuse the two; for engine screen States see the engine-state skill.
---

# Battlescape BattleStates (`*BState`)

The tactical layer runs its own action sub-machine, distinct from the engine `State` stack.
`BattlescapeGame` ([BattlescapeGame.h](../../../src/Battlescape/BattlescapeGame.h),
[BattlescapeGame.cpp](../../../src/Battlescape/BattlescapeGame.cpp)) owns a
`std::list<BattleState*> _states` — a **queue** of in-progress actions. Each `BattleState`
([BattleState.h](../../../src/Battlescape/BattleState.h)) represents one atomic, possibly
multi-cycle action (a unit walking a path, a projectile flying, an explosion animating).

> Terminology: a `BattleState` is **not** an engine `State`. `BattlescapeGame` itself is created by
> `BattlescapeState` (which *is* an engine `State`). The `*BState`s sequence what happens *inside*
> a running battle.

## The base class contract ([BattleState.h](../../../src/Battlescape/BattleState.h))

Every `*BState` subclasses `BattleState` and overrides some of:

| Method | When it runs | Responsibility |
|---|---|---|
| `init()` | once, when the state reaches the **front** of the queue | validate/prepare; spend TUs; may immediately `_parent->popState()` if nothing to do |
| `think()` | every cycle while this state is at the front (via `handleState()`) | advance the action one step; call `_parent->popState()` when finished |
| `cancel()` | on a cancel request | abort cleanly |
| `deinit()` | when the state is popped | cleanup/aftermath hooks |
| `getAction()` | — | returns the `BattleAction` (carries actor, type, target, TU cost, and `result` error string) |

The base implementations are empty ([BattleState.cpp](../../../src/Battlescape/BattleState.cpp)) —
override only what you need. The constructor takes `BattlescapeGame *parent` and usually a
`BattleAction` (copied into `_action`).

## The queue API (on `BattlescapeGame`)

| Call | Effect |
|---|---|
| `statePushBack(bs)` | enqueue at the back; if the queue was empty, `init()`s it immediately (this is the usual "do this action" call). Passing `0` (nullptr) enqueues the **end-turn sentinel** |
| `statePushFront(bs)` | push to front and `init()` now — interrupt with a higher-priority action |
| `statePushNext(bs)` | insert right **after** the current front — a follow-up that runs next |
| `popState()` | the front action finished: pop it (deferred-delete), run its aftermath, advance the queue. **A BState signals completion by calling this** |
| `handleState()` | called by the loop each cycle: `think()`s the front state (or, if front is the `0` sentinel, triggers `endTurn()`) |

### Two critical conventions

1. **A `*BState` ends itself by calling `_parent->popState()`** (from `init()` or `think()`),
   never by self-deleting. Deletion is deferred via `_deleted`, exactly like the engine stack — so
   `this` survives the rest of the cycle.
2. **The `0`/nullptr front element is the "end turn" request.** `handleState()`/`statePushBack`
   check `_states.front() == 0` and call `endTurn()`. Don't dereference the front without
   accounting for this sentinel.

`popState()` ([BattlescapeGame.cpp](../../../src/Battlescape/BattlescapeGame.cpp)) is where the
*aftermath* of an action lives: it reads the finished action's `result` (showing a warning to the
player on failure), rehandles the cursor/targeting mode for player units, checks for pending
actions on the actor, and drives AI continuation. When adding a new action type, decide whether
its post-action consequences belong in your `think()`, your `deinit()`, or `popState()`.

## Existing `*BState`s (sequence patterns to copy)

`UnitWalkBState`, `UnitTurnBState`, `ProjectileFlyBState`, `ExplosionBState`, `MeleeAttackBState`,
`PsiAttackBState`, `UnitDieBState`, `UnitFallBState`, `UnitPanicBState`
([src/Battlescape/](../../../src/Battlescape/)). Study how they **chain**: e.g. a shot is a
`ProjectileFlyBState` that, on impact, `statePushNext`/`statePushBack`s an `ExplosionBState`, which
may in turn enqueue a `UnitDieBState`. `UnitTurnBState` is the smallest, cleanest example to model a
new one on.

## Adding a new `*BState`

1. **Header** `src/Battlescape/MyActionBState.h`: copy the license header, `#include "BattleState.h"`,
   subclass `BattleState`, declare members + constructor (take `BattlescapeGame*` and a
   `BattleAction`), and `override` the lifecycle methods you need.
   ```cpp
   class MyActionBState : public BattleState {
       BattleUnit *_unit;
   public:
       MyActionBState(BattlescapeGame *parent, BattleAction action);
       ~MyActionBState();
       void init() override;
       void think() override;
       void cancel() override;
   };
   ```
2. **Source** `MyActionBState.cpp`:
   - `init()`: validate (actor alive? enough TUs?), spend TUs, set up animation/iteration state.
     If the action can't proceed, set `_action.result = "STR_..."` and `_parent->popState()`.
   - `think()`: advance one increment per cycle; when complete, `_parent->popState()`.
   - Reach engine/state via `_parent` (`_parent->getSave()`, `_parent->getMap()`,
     `_parent->getCurrentAction()`, etc.).
3. **Enqueue it** from wherever the action originates (usually in `BattlescapeGame` action handling
   or the AI), via `statePushBack(new MyActionBState(this, action))`.
4. **Build registration (REQUIRED, no globbing):** add the `.cpp`/`.h` to
   [src/CMakeLists.txt](../../../src/CMakeLists.txt) (`*_src` list), `src/OpenXcom.2010.vcxproj`,
   and `src/OpenXcom.2010.vcxproj.filters`.
5. If this is a **DX** gameplay addition, log it in [DX-Features.md](../../../DX-Features.md)
   (CLAUDE.md project rule).

## Gotchas

- **Don't `delete` a BState** — signal completion with `_parent->popState()`; the queue handles
  deferred deletion via `_deleted`.
- **TU accounting** belongs in `init()` (spend up front), and remember player vs AI (`FACTION_PLAYER`)
  differences that `popState()` already keys off — check there before duplicating logic.
- **The end-turn sentinel (`0`)** lives in the same queue; account for it when iterating/inspecting.
- **`init()` runs when the state reaches the front**, which for `statePushBack` on a non-empty queue
  is *later*, not at construction — don't assume members touched only in `init()` are set right
  after `new`.
- **Multi-cycle vs instant**: if your action completes instantly, you can `popState()` straight from
  `init()`; if it animates, drive it from `think()` and only pop on the final frame.
- This is **not** the engine `State` stack — `_parent->popState()` (BattlescapeGame) and
  `_game->popState()` (Game) are different machines. See engine-state for the latter.
