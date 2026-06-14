---
name: engine-state
description: >-
  Work with the engine's State stack — the stack-based state machine that drives every
  screen, window, and modal dialog in OpenXcom DX. Use when pushing/popping/replacing
  States, reasoning about State lifecycle (constructor vs init/think/handle/blit/destructor),
  passing data between screens, building transitions, modal dialogs, returning to a previous
  screen, or debugging "why does init() run again" / use-after-free / palette-on-resume issues.
  This is the control-flow layer; for building the *contents* of a screen (widgets + interface
  colors) see the game-ui-screen skill. For the Battlescape action sub-machine (*BState), see
  the battlescape-battlestate skill — that is a SEPARATE machine, not this one.
---

# The engine State stack

`Game` ([src/Engine/Game.h](../../../src/Engine/Game.h), [Game.cpp](../../../src/Engine/Game.cpp))
owns a `std::list<State*> _states` — the **state stack**. The top state is the active screen.
`Game::run()` is the single game loop: each cycle it `init()`s the top state if needed, polls SDL
events to the top state's `handle()`, calls `think()`, and `blit()`s. Each `State`
([State.h](../../../src/Engine/State.h)) is one full screen/window/dialog.

This skill is about the **stack mechanics and State lifecycle**. Authoring a screen's widgets and
its `interfaces.rul` styling is the *game-ui-screen* skill; the two compose (a UI screen IS a
`State`).

## The three stack operations (all on `Game`)

| Call | Effect | Use for |
|---|---|---|
| `_game->pushState(s)` | Pushes `s` on top; old state stays underneath | Opening a sub-screen / modal dialog you'll return from |
| `_game->popState()` | Removes the top state (deferred-deletes it) | Closing the current screen ("OK"/"Cancel" handlers) |
| `_game->setState(s)` | Pops **all** states, then pushes `s` | Hard transition / resetting the stack (e.g. into Geoscape, quit-to-menu) |

`pushState`/`popState`/`setState` all clear the `_init` flag so the new top state gets `init()`-ed
on the next cycle.

### Deferred deletion — the critical safety rule
`popState()` does **not** delete immediately. It moves the state into `_deleted`, which is freed at
the **start of the next cycle** (`Game::run`), so the transition is seamless and a state can pop
itself from inside its own handler. Consequences:

- **A state may pop itself**: `void FooState::btnOkClick(Action*) { _game->popState(); }` is correct
  and common — `this` stays alive for the rest of the current cycle.
- **Do not use a State pointer after you popped it** later in the same logic; assume it dies next
  cycle.
- States `add()`-ed surfaces are owned/freed by the State; never `delete` child surfaces yourself.

## State lifecycle — what runs when

```
new FooState(...)   // CONSTRUCTOR: build widgets, setInterface(), add(), wire handlers, ONE-TIME setup
        │            //  (data shown here is a snapshot at construction time)
   pushState
        │
   init()           // EVERY time this state (re)becomes the top of the stack — including when a
        │            //  state pushed ON TOP of it later pops off. Put "refresh on (re)entry" here:
        │            //  re-read savegame data, repopulate lists, update labels. Call State::init() first.
   handle(action)   // per SDL event, only the TOP state (unless a lower state opts in)
   think()          // every cycle (animations, timers, polling)
   blit()           // every cycle, render
        │
   popState  →  ~FooState()   // destructor usually EMPTY; child surfaces auto-freed
```

The single most common mistake: **putting refresh logic in the constructor instead of `init()`.**
If a screen shows stale data after you return to it from a sub-screen, the fix is almost always to
move that update into an overridden `init()` (remember `State::init()` first). Example:
[BaseInfoState::init()](../../../src/Basescape/BaseInfoState.cpp) re-reads base stats every time.

## Passing data between States

- **Forward (open a screen with context)**: pass via constructor args, e.g.
  `_game->pushState(new TransfersState(_base))`. States hold raw pointers into the `Mod`
  (rules) and `SavedGame` (mutable state) — both reachable via `_game->getMod()` /
  `_game->getSavedGame()`.
- **Backward (return a result)**: there is no return value. Either (a) mutate shared
  `SavedGame`/domain objects the parent will re-read in its `init()`, or (b) pass the parent
  `this` (or a callback target) into the child's constructor so the child can call back before
  it pops. Pattern (a) + parent `init()` refresh is the idiomatic default.

## Minimal new State (control-flow skeleton)

```cpp
// FooState.h
#pragma once
#include "../Engine/State.h"
namespace OpenXcom { class TextButton;
class FooState : public State {
	TextButton *_btnOk;
public:
	FooState(/* context pointers */);
	void init() override;            // omit if nothing refreshes on (re)entry
	void btnOkClick(Action *action);
};
}
```
```cpp
// FooState.cpp (constructor: see game-ui-screen for the widget/setInterface/add details)
FooState::FooState() {
	_btnOk = new TextButton(288, 16, 16, 176);
	setInterface("foo");
	add(_btnOk, "button", "foo");
	centerAllSurfaces();
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&FooState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&FooState::btnOkClick, Options::keyCancel);
}
void FooState::init() { State::init(); /* refresh data shown each time visible */ }
void FooState::btnOkClick(Action*) { _game->popState(); }   // close & return
```

## Gotchas

- **`init()` re-runs on resume**, not just first show. Make it idempotent (don't append to lists
  without clearing, don't re-add surfaces).
- **`_game` is a `static Game*` on `State`** — every State shares it; available in the constructor.
- **Palette is per-State.** On resume the engine restores the top state's palette; if you push a
  state with a different palette and pop back, that's handled — but if you blit using another
  state's palette you'll get garbled colors (see game-ui-screen palette notes).
- **Full-screen vs overlay**: a pushed state draws over the one below. Mark truly full-screen
  states (most are) so lower states aren't needlessly redrawn; small dialogs (`ErrorMessageState`,
  confirmations) are pushed on top and pop back to reveal the screen beneath.
- **Modal input**: use `setModal(surface)` to capture input within a state; only the top state
  receives `handle()` by default.
- **`setState()` deletes everything below** — never hold a pointer to a state across a `setState`.
- New State source files must be registered in **CMakeLists.txt + the .vcxproj + the
  .vcxproj.filters** (no globbing) — see game-ui-screen for exact locations.
