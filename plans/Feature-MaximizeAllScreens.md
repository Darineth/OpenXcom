# Feature - Maximize (almost) all UI screens

**Status:** ✅ Implemented (Jun 2026) — central `ScaleContext` approach.

## Outcome (as shipped)

A central rule now drives the base resolution on every top-of-stack change, replacing the old
per-screen save/restore hack:

- `State` gained `enum class ScaleContext { UI, Geoscape, Battlescape, SelfManaged }` and a
  `virtual getScaleContext()` (default `UI`) — [State.h](../src/Engine/State.h).
- `Game::applyDisplayScale(State*)` runs in `Game::run`'s `!_init` branch, right before the new
  top state's `init()` — [Game.cpp](../src/Engine/Game.cpp). It is a **no-op when
  `maximizeInfoScreens` is off** (vanilla behavior). It picks the base resolution from the context
  of the **bottom-most full-screen state of the visible blit group** (walk `_states` back-to-front
  to the first `isScreen()`), not the literal top state, so a `_screen=false` overlay (on-globe
  dialogs, `PauseState`, `BuildNewBaseState`) inherits the primary view it draws over instead of
  forcing the globe to 320:
  - `UI` → 320×200, recentering the top state's surfaces by the resolution delta (see below).
  - `Geoscape` → geoscape scale. `Battlescape` → battlescape scale. `SelfManaged` → left untouched.
- Overrides: `GeoscapeState`/`DogfightState` → `Geoscape`; `BattlescapeState`/`NextTurnState` →
  `Battlescape`; `StartState`/`CutsceneState`/`SlideshowState`/`VideoState`/`TestState` →
  `SelfManaged`.
- **Recenter, don't rebuild:** `centerAllSurfaces()` runs in each `State` constructor using the
  resolution active *then*, so a later central scale change leaves surfaces offset. The `UI` branch
  fixes this by recentering the top state's surfaces by the delta and tracking the resolution they
  were last centered for in per-state `_layoutBaseX/_layoutBaseY` ([State.h](../src/Engine/State.h)).
- **resetDisplay uses the real screen size:** the decision to call `resetDisplay(false)` compares
  `want` against the screen's actual base (`Screen::getBaseWidth()/getBaseHeight()`), **not**
  `Options::baseXResolution`. `OptionsBaseState::btnOk/CancelClick` rewrites the global without
  resetting the display, so keying off the global would skip a needed reset and leave a resumed
  overlay (e.g. `PauseState` over the geoscape) rendered into a stale 320 buffer.
- The 6 battlescape popups (`UnitInfoState`, `ScannerState`, `MiniMapState`, `MedikitState`,
  `AlienInventoryState`) and `InventoryState` override `getScaleContext()` dynamically to return
  `UI` when the option is on (so they maximize) and `Battlescape` when it's off (so they keep the
  tactical scale they overlay — avoiding open/close display thrash, and covering both the in-battle
  and base/craft-equip inventories). Their old manual maximize/restore blocks were removed; the
  inventory's gameplay teardown (gravity/lighting/FOV) was preserved.

### Deviations from the original plan

- A 4th context `SelfManaged` was added for the loading/cutscene/intro/test screens (the plan
  called them "exempt"); `IntroState` doesn't exist (intros run through `VideoState`).
- The 6 popups + inventory did **not** simply fall through to the `UI` default. To avoid a
  regression when `maximizeInfoScreens` is **off** (they used to inherit the *battlescape* scale,
  not the geoscape scale), they return `Battlescape` in the off case via a dynamic override.
- The scattered geoscape-restore `updateScale` calls in transition/click handlers (`GoToMainMenuState`,
  `SaveGameState`, `AbandonGameState`, `BriefingState`, `DebriefingState`, `OptionsBaseState`, …)
  were left in place: they are now redundant because the central rule re-applies the correct
  resolution before each `init()`, but they remain harmless (and give constructors a sane
  resolution for building widgets before the central rule runs).

---

## Problem

OXCE's `Options::maximizeInfoScreens` drops a screen to 320×200 so its UI fills the window instead
of sitting small in a corner at a high base resolution. But only **6 Battlescape popups** honor it
(`UnitInfoState`, `ScannerState`, `MiniMapState`, `MedikitState`, `InventoryState`,
`AlienInventoryState`). Every other menu/detail/dialog screen — main menu, options, all of
Basescape, Ufopaedia, debrief, etc. — inherits the **geoscape** base resolution
([MainMenuState.cpp:54](../src/Menu/MainMenuState.cpp#L54)) and renders small when the geoscape scale
is large.

**Goal (per user):** when `maximizeInfoScreens` is on, **every** screen maximizes to 320×200
*except* the primary gameplay views — Battlescape, Geoscape, and screens that overlay them at the
gameplay resolution.

## How base resolution works today

- `Options::baseXResolution`/`baseYResolution` is the global internal render size.
  `Screen::updateScale(scaleType, baseX, baseY, change)` ([Screen.cpp:668](../src/Engine/Screen.cpp#L668))
  computes it from a scale setting (`SCALE_ORIGINAL` = 320×200, `SCALE_SCREEN`, `SCALE_2X`, …).
- It's applied per context: battlescape → `battlescapeScale`/`baseX/YBattlescape`; everything else
  (geoscape **and menus**) → `geoscapeScale`/`baseX/YGeoscape` (`Game.cpp:234-235`,
  `MainMenuState.cpp:54`).
- The 6 info popups override to `SCALE_ORIGINAL` on entry and restore the battlescape scale on exit.

## Approach — central, opt-out (recommended)

Replace the per-screen hack with one central rule applied whenever the top of the state stack
changes (the `!_init` branch at [Game.cpp:163](../src/Engine/Game.cpp#L163), which already calls the
new top state's `init()` exactly once per transition).

1. **Add a scale-context to `State`:**
   ```cpp
   enum ScaleContext { SCALE_UI, SCALE_GEOSCAPE, SCALE_BATTLESCAPE };
   virtual State::ScaleContext getScaleContext() const { return SCALE_UI; } // default: a UI screen
   ```
   Primary views override it: `GeoscapeState` → `SCALE_GEOSCAPE`; `BattlescapeState` → `SCALE_BATTLESCAPE`;
   geoscape overlays (`DogfightState`) → `SCALE_GEOSCAPE`; self-managing full-display screens
   (`StartState`, `CutsceneState`/`SlideshowState`, `VideoState`, `IntroState`) → exempt (see below).

2. **Apply resolution centrally** in `Game::run`'s top-of-stack hook (a small helper
   `Game::applyDisplayScale(State*)` called right before `init()`), choosing the base resolution from
   the top state's context:
   - `SCALE_UI` → `SCALE_ORIGINAL` (320×200) **if** `maximizeInfoScreens`, else `geoscapeScale`
     (today's behavior).
   - `SCALE_GEOSCAPE` → `geoscapeScale`.
   - `SCALE_BATTLESCAPE` → `battlescapeScale`.

   Track the currently-applied mode and only call `resetDisplay()` when it actually changes, so
   pushing/popping same-context screens (the common case) costs nothing.

3. **Remove the 6 per-screen maximize blocks** — they become the `SCALE_UI` default automatically,
   and popping back to Battlescape restores `battlescapeScale` via `BattlescapeState`'s context. (The
   inventory's base-craft path that already special-cases scale needs checking — see Risks.)

### Exempt ("don't force 320×200") set — needs confirmation

- **Primary views:** `BattlescapeState` (`SCALE_BATTLESCAPE`), `GeoscapeState` (`SCALE_GEOSCAPE`).
- **Geoscape overlays:** `DogfightState` (`SCALE_GEOSCAPE`).
- **Self-managing display:** `StartState`, `CutsceneState`, `SlideshowState`, `VideoState`,
  `IntroState`, `TestState` — leave their display alone (treat as exempt / their own context).

Everything else (Menu, Basescape, Ufopaedia, Battlescape *dialogs* like Briefing/Debriefing/NextTurn,
geoscape *dialogs* like UfoDetected/Intercept) defaults to `SCALE_UI` → maximizes.

## Risks / things to verify

- **Display thrash / flicker** on transitions — mitigated by only `resetDisplay()` on actual mode
  change; verify no flicker pushing dialogs over geoscape/battlescape.
- **The inventory base-craft branch** (`InventoryState` has an `isBaseCraftInventory()` path that
  picks geoscape vs battlescape scale) must keep working when its manual block is removed.
- **Palette / `resetDisplay(false)`** correctness on each transition (the per-screen code passes
  `false`).
- **Screens that assume a fixed resolution** for layout math — most use 320×200 anyway; spot-check
  oversized geoscape/basescape screens.
- **`maximizeInfoScreens` default** — currently off on some platforms; behavior only changes when on.

## Key code (planned)

| File | Role |
|------|------|
| `src/Engine/State.h` | `ScaleContext` enum + `virtual getScaleContext()` (default `SCALE_UI`) |
| `src/Engine/Game.cpp` | `applyDisplayScale(top)` helper called from the `!_init` top-of-stack hook |
| `src/Geoscape/GeoscapeState`, `DogfightState`, `src/Battlescape/BattlescapeState` | override `getScaleContext()` |
| `src/Menu/StartState`/`CutsceneState`/`SlideshowState`/`VideoState` | exempt handling |
| The 6 existing info popups | remove their manual maximize/restore blocks |
