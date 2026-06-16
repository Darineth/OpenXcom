# Feature - Action Menu Revamp

**Status:** ✅ Implemented. Compact layout, on-row hotkey labels (+ 6th key), affordability
flags (out-of-TU / out-of-ammo, red) and an extra partial-ammo warning (amber), and shot/pellet
counts are all live. Refinements added during implementation: the unaffordable **frame** also
recolors (not just text); a third **amber "partial ammo" state** when there's some ammo but
fewer rounds than a full burst; and DX options were relocated to a dedicated **DX** owner/tab
(see "DX options" below). The effective-range readout remains deferred to Phase 5.

## Overview

The battlescape **action menu** is the popup that appears when you click a unit's weapon/item
(or use the left/right-hand action keys): a vertical stack of boxes, one per available action
(Throw, Snap Shot, Aimed Shot, Auto Shot, Prime, Hit, Use, …), each showing the action name,
accuracy, and TU cost. This feature revamps that popup to be **more compact and more
informative** without changing what the actions *do*.

Scope (from the Phase 1 checklist item):

1. **More compact layout** — smaller font, shorter rows, so the popup takes less of the screen.
2. **Numeric action hotkeys with on-popup labels** — show each action's configurable hotkey on
   its row, and ensure every visible row is reachable by a key.
3. **Affordability indicators** — visibly mark an action the unit can't currently perform
   because it's **out of TU** or **out of ammo**.
4. **Shot-count display** — for multi-projectile modes (auto/burst, shotgun) show how many
   shots/pellets the action fires.

**Out of scope (deferred):** the **effective-range readout** — it depends on the Phase 5
aim-cone model and is tracked there, not here.

## Motivation

The vanilla popup is bulky (big font, 40px rows) and silent about three things the player
otherwise has to infer: which key triggers each action, whether the unit can actually afford the
action, and how many projectiles a burst/shotgun shot throws. Surfacing these removes
guesswork and misclicks (e.g. picking Auto with too few TU, or not realizing a weapon is dry).

## Current implementation (what we're changing)

| File | Role today |
|------|-----------|
| `src/Battlescape/ActionMenuState.h/.cpp` | Builds the popup: owns `ActionMenuItem _actionMenu[6]`, `addItem(ba, name, &id, key)` computes accuracy + TU and binds the per-action key. `handle()` closes on right-click/cancel. |
| `src/Battlescape/ActionMenuItem.h/.cpp` | One row. `InteractiveSurface(272, 40, x+24, y-(id*40))`, big font, frame thickness 8. Columns: `_txtDescription` (200×20 @10,13), `_txtAcc` (100×20 @140,13), `_txtTU` (80×20 @210,13). `setAction(action, description, accuracy, timeunits, tu)`. |
| `src/Battlescape/SkillMenuState.h/.cpp` | **Subclass** that reuses `ActionMenuItem`/`addItem`/`setAction` for the skill popup. Any signature/layout change must keep this working. |
| `src/Engine/Options.*` | `keyBattleActionItem1..5` (defaults `SDLK_1..5`), `OPTION_OXCE`, category `STR_BATTLESCAPE`. |
| `bin/standard/xcom1/interfaces.rul` (+ xcom2) | `battlescape` interface, `actionMenu` element → `color` (text), `color2` (highlight base), `border` (frame). |

Key facts:
- **6 slots, 5 keys.** `_actionMenu[6]` but only `keyBattleActionItem1..5`. The 6th slot is
  unreachable by keyboard today.
- **Hotkeys are semantic, not positional.** `addItem` binds a key chosen by action type
  (e.g. Auto→item3, Snap→item2, Aimed/Launch/Prime→item1, Melee→item4, Throw→item5), so the
  same key means the same action regardless of where it lands in the list. The key is **not**
  drawn on the row.
- Actions are always shown and always clickable; affordability is only revealed *after* clicking
  (a `warning()` such as `STR_NOT_ENOUGH_TIME_UNITS` / out-of-ammo), via
  `SavedBattleGame::canUseWeapon` and `BattleUnit::getActionTUs`/`getTimeUnits`.

## Design

### Layout (ASCII mockup)

**Current** — `ActionMenuItem` 272×40, big font, 3 columns, tall rows; no hotkey label, no shot
count, no affordability cue:

```
                  (rows stack upward from the click point)
   ┌──────────────────────────────────────────┐  ┐
   │                                            │  │
   │   Auto Shot          Acc>52%    TUs>26     │  │ 40px row
   │                                            │  │ (big font,
   ├──────────────────────────────────────────┤  ┘  centered)
   │                                            │
   │   Snap Shot          Acc>65%    TUs>18     │
   │                                            │
   ├──────────────────────────────────────────┤
   │                                            │
   │   Aimed Shot         Acc>110%   TUs>50     │
   │                                            │
   ├──────────────────────────────────────────┤
   │                                            │
   │   Throw              Acc>72%    TUs>25     │
   │                                            │
   └──────────────────────────────────────────┘
    └──────────────── 272px ───────────────────┘
   thick 8px frame · no key · no ×shots · no affordability cue
```

**Proposed** — small font, ~25px rows, key label at left, `×N`/pellet column, unaffordable rows
dimmed with a reason tag (still clickable):

```
   ┌────────────────────────────────────────────────┐  ┐
   │ 3  Auto Shot   ×3        Acc>52%  TUs>26         │  │ ~25px row
   ├────────────────────────────────────────────────┤  ┘ (small font)
   │ 2  Snap Shot             Acc>65%  TUs>18         │
   ├────────────────────────────────────────────────┤
   │ 1  Aimed Shot            Acc>110% TUs>50   No TU │ ← dimmed/red
   ├────────────────────────────────────────────────┤
   │ 5  Throw                 Acc>72%  TUs>25         │
   └────────────────────────────────────────────────┘
     │  └── description ──┘└─×shots─┘└─acc─┘└─tu─┘└status┘
     └ hotkey label (semantic key, e.g. "3" = Auto)
```

**Shotgun / out-of-ammo example** — pellets shown separately, no-ammo flagged:

```
   ┌────────────────────────────────────────────────┐
   │ 3  Auto Shot   ×3 (9 pellets)  Acc>40% TUs>30   │
   ├────────────────────────────────────────────────┤
   │ 2  Snap Shot   (3 pellets)  Acc>55% TUs>20  No Ammo │ ← dimmed
   └────────────────────────────────────────────────┘
```

Summary of changes:

| | Current | Proposed |
|---|---|---|
| Row height | 40px, big font | ~25px, small font |
| Hotkey | bound but invisible | shown at left (`3`, `2`, `1`, `5`…) |
| 6th slot | unreachable (no key) | `keyBattleActionItem6` added |
| Multi-shot | not shown | `×N` column |
| Shotgun | not shown | `(N pellets)` shown separately |
| Can't afford | only after clicking | row dimmed + `No TU`/`No Ammo`, still clickable |

### 1. Compact layout
- Shrink `ActionMenuItem` to a small-font row. Proposed geometry: height **~25px** (from 40),
  frame thickness **~3** (from 8), text widgets switched from `setBig()` to small font, vertical
  text offset reduced to center within the shorter row. Stack offset becomes `y - id*height`.
- Keep width 272 (room for the new columns) and the `x+24` origin.
- Re-tune highlight: `_highlightModifier` already differs for TFTD; verify the thinner frame
  still reads as highlighted on hover.
- **Always-on** (decision A): the compact layout fully replaces the vanilla popup; no toggle
  option.

### 2. Numeric hotkeys + on-popup labels
- Draw the bound key glyph at the **left of each row** (new `_txtKey`, e.g. "1", a few px wide),
  shifting the description right.
- Keep the **semantic** key model (muscle memory: a key always means the same action) and simply
  surface the label. Resolve the glyph from the `SDLKey` passed into `addItem` via
  `SDLK_to_str`/`Unicode`-style name, falling back to blank when a slot has no key.
- **Add `keyBattleActionItem6`** (default `SDLK_6`) so the 6th slot is reachable and labelable.
  `SkillMenuState` already supplies its own `hotkeys` list — extend it to cover the 6th slot too.
- Decision B: **semantic keys + labels** (not positional).

### 3. Affordability indicators (out of TU / out of ammo)
- In `addItem`, compute two booleans for the action:
  - **out of TU:** `_action->actor->getTimeUnits() < tu` (tu already computed there). Mind also
    energy if the action has an energy cost — start with TU only, note energy as a follow-up.
  - **out of ammo:** for the firing/throwing/melee types, `!_action->weapon->getAmmoForAction(ba)`
    (same emptiness test used elsewhere). Throw/prime/use without ammo concept → never "no ammo".
- Render an unaffordable row **dimmed / in a warning color**, and append a short reason tag
  (e.g. `STR_ACTION_NO_TU`, `STR_ACTION_NO_AMMO`) in the TU/Acc area.
- **Still clickable** (decision C): keep the row clickable (vanilla behavior — clicking shows the
  existing warning), just visually flagged.
- Needs a palette-correct "unaffordable" color for the `actionMenu` element (new ruleset color
  slot, e.g. `actionMenuDisabled`). Pick via the `ui-palette-colors` skill against the
  battlescape palette.

### 4. Shot-count display
- For `BA_SNAPSHOT/BA_AIMEDSHOT/BA_AUTOSHOT` read `getConfig{Snap,Aimed,Auto}()->shots`; show a
  `×N` indicator when `shots > 1` (so single-shot snap/aimed stay clean).
- For **shotgun** ammo, read the loaded ammo's `getShotgunPellets()`; when `> 0` show pellet
  count. An auto shotgun fires `shots × pellets` projectiles — display both, e.g. `×3 (9 pellets)`
  (decision D: ×N with pellets shown separately, not a combined total).
- Add a small `_txtShots` column (or fold into the description, e.g. "Auto Shot ×3").

### Shared-code / SkillMenuState safety
- Extend `ActionMenuItem::setAction` with the new optional fields (key glyph, shot count,
  affordability/reason) using **defaulted parameters**, or add small dedicated setters
  (`setHotkey`, `setShots`, `setAffordable`). Defaulted setters keep `SkillMenuState` (which has
  no ammo/shot concept) compiling unchanged — it just won't set them.
- `_actionMenu[6]` size and the new 6th key must be handled in both `ActionMenuState` and
  `SkillMenuState` construction loops (both already iterate `std::size(_actionMenu)`).

## New/changed files (anticipated)
- `ActionMenuItem.h/.cpp` — new geometry, small font, `_txtKey`/`_txtShots`, affordability color,
  extended `setAction`/setters, `draw()`/`setPalette()` updates.
- `ActionMenuState.cpp` — `addItem` computes hotkey glyph, shot count, affordability; passes them
  in. Possibly read `actionMenuCompact` to choose geometry.
- `SkillMenuState.cpp` — adjust for any `setAction` change + 6th hotkey.
- `Options.inc.h` / `Options.cpp` — `keyBattleActionItem6`.
- `interfaces.rul` (xcom1 + xcom2) — `actionMenuDisabled` color slot (and verify compact metrics).
- `bin/common/Language/DX/en-US.yml` — new `STR_*` (reason tags, shot-count format) per CLAUDE.md.

## Localization strings (planned)
- `STR_ACTION_NO_TU` — short "not enough TU" tag (e.g. "No TU").
- `STR_ACTION_NO_AMMO` — short "out of ammo" tag (e.g. "No Ammo").
- `STR_ACTION_SHOTS_SHORT` — shot-count format (e.g. "x{0}").
- `STR_ACTION_PELLETS_SHORT` — pellet-count format (e.g. "{0} pellets"), if shown separately.
- `STR_ACTION_ITEM_6` — options label for the new 6th action key.

## Resolved decisions
- **A. Compact layout:** always-on, no toggle option.
- **B. Hotkey model:** semantic per-action keys + on-row labels; add a 6th key.
- **C. Unaffordable rows:** keep clickable, just visually flagged (+ reason tag).
- **D. Shot-count format:** `×N` for multi-shot, with shotgun pellets shown separately
  (e.g. `×3 (9 pellets)`), not a combined total.

## As-built notes (deviations / additions)
- **Row geometry:** 272×25 (from 40), small font, frame thickness 3. The reason tag reuses the
  shot-count column (`_txtShots`) — an unaffordable action's shot count is moot.
- **Frame recolors too:** `setUnaffordable`/`setPartialAmmo` recolor the `Frame` (via `_frame->
  setColor`), not just the text, so the whole row border turns red/amber.
- **Third state — partial ammo (amber):** when there's *some* ammo but fewer rounds than a full
  multi-shot burst (`shots > 1 && available < shots * spendPerShot`, but `>= spendPerShot`), the
  row recolors to a warning amber (`actionMenuWarning`) and keeps the `×N`. Self-powered weapons
  (return themselves from `getAmmoForAction`) are never flagged; single-shot modes are
  all-or-nothing. Read via `getActionConf(ba)->shots`/`spendPerShot` + ammo `getAmmoQuantity()`.
- **Chosen colors:** `actionMenuDisabled` = 35 (UFO) / 11 (TFTD) red; `actionMenuWarning` =
  16 (UFO) / 160 (TFTD) amber. Code falls back to `color2` / disabled if an element is absent.
- **DX options:** rather than registering under OXCE, `combatLogEnabled` and `keyBattleActionItem6`
  now live in new `createAdvancedOptionsDX` / `createControlsDX` functions under a new `OPTION_DX`
  owner, surfaced as a dedicated **DX** tab in Options → Advanced and Options → Controls. The
  generic `OTHER` fork-template slot (enum value, empty `*OTHER` stubs, hidden `_btnOTHER`) is left
  in place untouched. The DX tab shares the hidden OTHER button's x-slot. String: `STR_ENGINE_DX`.
- **6th key:** `keyBattleActionItem6` (default `6`) is configurable and labeled; the built-in
  actions still use the 1–5 semantic bindings, so it's mainly for the 6-slot popup / skills.

## Testing
No unit tests (per project). Verify in a battle: popup is compact and readable; each row shows
its key; pressing the key fires the right action; rows for unaffordable actions are flagged and
the warning still appears on click; auto/burst show `×N`, shotgun shows pellets; `SkillMenuState`
popup still works; both UFO and TFTD palettes look right.
