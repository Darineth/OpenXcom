# OpenXcom DX — Features

This document tracks features added in OpenXcom DX on top of OXCE-Plus. Entries will be added
here as features are implemented.

## Combat Log

A floating, centered event log across the top of the Battlescape that reports combat events
as they happen, color-coded by outcome (good/neutral/warning/bad from the player's
perspective). Reported events: new turn; weapon fire and throws (with shot mode, e.g. "fires
Rifle (Snap Shot)"); melee attacks; reaction shots/swings (tagged as such); hits, with
research-gated damage detail (exact damage and fatal wounds for your units and researched
enemies, a vague light/heavy band for un-researched enemies); kills and knockouts; panic and
berserk; and a player-only warning when a shot empties a weapon. Entries fade out over time and
the list is capped, so it self-empties when combat is quiet. Unit and weapon names respect the
player's knowledge — own soldiers and researched enemies are named, while unseen/un-researched
hostiles show as "Hostile"/"Unknown" and their gear as "an unknown weapon". The log is
transient (never serialized) and distinct from the on-demand OXCE hit log (Ctrl-H).

- Toggle: advanced option **Combat log** (`combatLogEnabled`, default on), under Battlescape.
- Theming: the `combatLog` element in the `battlescape` interface ruleset sets the panel's
  position and size; `combatLogNeutral` / `combatLogGood` / `combatLogWarning` / `combatLogBad`
  set the four outcome colors.

## Action Menu Revamp

The battlescape action popup (Throw / Snap / Aimed / Auto / Hit / …) is more compact and more
informative. Rows use a small font in ~25px boxes, and each row now shows its configurable
hotkey at the left. Multi-shot modes show a shot count (e.g. `x3`) and shotgun ammo shows pellets
(`x3 (9 pellets)`). Affordability is flagged at a glance: an action you can't perform — not
enough TU, or no usable ammo — turns the whole row (text + border) red with a "No TU" / "No Ammo"
tag, while an action with *some* ammo but not enough for a full burst turns amber. Flagged rows
stay clickable (the usual warning still fires).

- A sixth configurable action hotkey (`keyBattleActionItem6`, default `6`) was added so the
  six-slot popup and the skill menu are fully keyboard-reachable.
- Theming: `actionMenuDisabled` (red) and `actionMenuWarning` (amber) elements in the
  `battlescape` interface ruleset color the unaffordable / partial-ammo states.
- DX options now live under their own **DX** tab in Options → Advanced and Options → Controls
  (a dedicated `OPTION_DX` owner), separate from the OXC/OXCE tabs.

## On-Map Overlays

A family of small, always-on visual cues drawn directly onto the Battlescape map, each with its
own toggle so they can be enabled independently. *(design:
[plans/Feature-MapOverlays.md](plans/Feature-MapOverlays.md))*

- **Hovered unit name** — a knowledge-aware, faction-colored floating name label over the unit
  under the cursor. Toggle: **Hovered unit name** (`hoveredUnitNameEnabled`, default on).
- **Primed-grenade indicator** — a pulsing marker hovering over each player-thrown primed
  grenade lying on a discovered tile (a brightness-pulsed red disc for normal grenades, an
  animated red "wifi ping" for proximity grenades). Enemy grenades are never shown. Toggle:
  **Primed grenade indicator** (`grenadeIndicatorEnabled`, default on).
- **Unit status indicators** — status glyphs hovering over each of the player's own *living*
  units, one per active condition: **bleeding** (fatal wounds), **on fire**, **shock** (losing
  HP each turn), and a **near-knockout** stun warning when accumulated stun is close to dropping
  the unit. All active conditions stack side-by-side above the head. Uses the same glyphs the
  engine already shows over unconscious bodies. Toggle: **Unit status indicators**
  (`unitStatusIndicatorEnabled`, default on).

  The four OXCE status indicators (`Floor{Wound,Burn,Shock,Stun}Indicator`) are mod-supplied and
  absent from base data, so they used to render nothing out of the box. DX now builds a small
  **procedural fallback** for all four in `Map::init()` (red blood drop / flame / lightning bolt /
  "Zzz" sleep glyph, palette-safe and shape-distinct), preferred only when no mod art is present — so both the
  living-unit overlay and the long-dormant unconscious-body indicators now work by default. A mod
  can still override any of them with an `extraSprites` `Floor*Indicator` (`singleImage: true`).
- **Motion-detector readings** — a pulsing amber target reticle painted on the floor of each
  enemy/neutral unit detected this turn by a motion scanner (a path-preview-style floor decal),
  brighter the more the unit moved. It shows passively (no key held) and is drawn under the unit/
  wall sprites, so a visible enemy occludes its own marker and the cue is left for fog tiles —
  replacing OXCE's hard-to-see Alt-held bobbing arrow. Detection is unchanged from OXCE — a unit
  only lights up once actually scanned, so the overlay grants no intel a scanner wouldn't. Toggle:
  **Motion detector readings on map** (`motionDetectorOverlayEnabled`, default on).

## Fog of War

Base OpenXcom draws every *discovered* tile at full brightness, with no on-map sign of what a
soldier can actually see right now. DX **dims discovered tiles that no player unit currently has in
line of sight**, so remembered terrain reads as out of live observation while in-sight terrain stays
bright (undiscovered tiles remain black). The dim layers on top of the existing light/night-vision
shading and updates live — tiles re-fog as soon as soldiers move or turn. *(design:
[plans/Feature-FogOfWar.md](plans/Feature-FogOfWar.md))*

Implemented as a renderer change keyed off the engine's existing per-tile player-LOS count
(`Tile::getVisible()`); also fixes a pre-existing double-increment in
`TileEngine::calculateTilesInFOV` that kept that count from returning to zero (so tiles never
re-fogged). Toggle: **Fog of war (dim unseen tiles)** (`fogOfWarEnabled`, default on).

## Maximize Info Screens (all screens)

OXCE's **Maximize Info Screens** option (`maximizeInfoScreens`) drops a screen to 320×200 so its
UI fills the window instead of sitting small in a corner at a high base resolution — but stock
OXCE only honored it for 6 Battlescape popups. DX makes it apply to **every** menu, detail, and
dialog screen (main menu, options, all of Basescape, Ufopaedia, briefings/debriefings, geoscape
dialogs, …). The only screens left at the gameplay resolution are the primary views themselves —
the Geoscape, the Battlescape, and dogfights — plus the self-managing loading/cutscene/intro
screens.

This is driven centrally rather than per-screen: each `State` declares a `ScaleContext`
(`UI` / `Geoscape` / `Battlescape` / `SelfManaged`), and `Game` applies the matching base
resolution once whenever the top of the state stack changes (only resetting the display when the
resolution actually changes, so the common case is free). When the option is **off**, behavior is
unchanged from before — info screens that overlay the Battlescape (the unit info / scanner /
minimap / medikit / inventory popups) keep the battlescape scale, so there's no open/close display
thrash. No new option; this extends the existing `maximizeInfoScreens` toggle. *(design:
[plans/Feature-MaximizeAllScreens.md](plans/Feature-MaximizeAllScreens.md))*

## Craft Stat Display

The Basescape craft info screen now shows the craft's core performance numbers alongside its
status readouts. The stats are laid out in two compact columns so they fit cleanly at 320×200:
damage capacity, maximum speed, and acceleration on the left; current damage, shield, and fuel on
the right. This mirrors the numbers already shown in the Ufopaedia craft article, but makes them
available directly from Craft Info where base management happens.

## Inventory UI Improvements

The inventory ground area now supports mouse-wheel scrolling anywhere over the ground slots
(`WHEEL UP` for backward, `WHEEL DOWN` for forward), alongside the existing ground-scroll button
and keyboard controls. Scrolling moves one column at a time (not page-by-page), and the view
starts fully scrolled left when a unit is selected. Multi-slot items that extend past the left
edge of the viewport are rendered with their visible portion showing.

## Soldier Info Layout Rework

The Soldier Info screen now follows the Legacy DX-style two-row action layout (excluding Level/EXP):
top row `<<`, `OK`, `>>`, `DIARY`, `ARMOR`, `SACK`; second row with a craft button and
`INVENTORY`. The craft line is now a clickable assign/unassign toggle for the base's first craft
slot (using the same placement validation rules as craft soldier assignment), and `INVENTORY`
opens base inventory setup with the current soldier pre-selected.

## Debriefing Soldier Results

The Battlescape debriefing now keeps each soldier in the mission-results table and shows a compact
outcome code for them: `KIA`, `MIA`, `WND`, or `OK`. Wounded soldiers also show recovery time in
days, while the existing per-soldier stat-gain breakdown remains on the same view. Bad outcomes and
the recovery-day value are highlighted in the list's secondary color.

## Geoscape Activity Display

A persistent text panel on the left side of the Geoscape showing real-time activity across all
bases. Updates on every game-time tick (5 s, 10 min, 30 min, 1 h, 1 day).

**Global section (top, shown before per-base data):**
- Economy warnings (reversed-color alert text): negative current balance ("In the red"), negative
  monthly net (income − maintenance = "Monthly deficit"), and a maintenance spike flag when the
  live maintenance exceeds last month's committed value by ≥ §100 k and ≥ 20%.

**Per-base sections (each base shown with its name as header):**
- Alien resource storage count.
- Research projects: translated name with (spent/cost) progress.
- Idle scientist alert (reversed color).
- Manufacturing queue: item name with (produced/total) or (inf), plus a `[sell]` tag when
  auto-sell is active.
- Idle engineer alert (reversed color).
- Training capacity: Martial Training (in-training/slots, plus queued count) and Psi Lab
  Training (in-training/slots) — shown only when training facilities exist.
- Facilities under construction: translated name and remaining days.
- Craft maintenance: repairs with ETA highlighted (reversed color), or refuelling/rearming.
- Active crafts (STR_OUT): craft name and detailed mission status (patrolling, intercepting UFO,
  returning, low fuel, mission complete, tailing, or destination name).
- Store warnings (reversed color): "Stores full" when items exceed capacity, "Stores near full"
  when craft/transfer items would push stores over.
- Base defense readiness (reversed color): count of disabled defense facilities, count of
  defense facilities lacking required ammo.
- Incoming transfers: count with nearest-arriving item name and ETA.
- Wounded soldiers: total count, severe-wound subset (reversed), and longest recovery time in days.

- Toggle: advanced option **Activity display** (`activityDisplayEnabled`, default on), under DX.

## Concurrent Projectile Flight

The engine can now render multiple projectiles in flight simultaneously. This is the
foundation for shotgun pellets flying as individual visible trajectories, burst-fire rounds
overlapping in the air, and dual-fire weapons firing both barrels at once. Each projectile
travels and renders independently, carries its own impact result so overlapping shots resolve
correctly, and the camera follows the centroid (average position) of all visible bullets.

### Timer-based firing cadence

Multi-shot actions (auto/spray bursts) no longer wait for the previous round to impact before
launching the next. Instead, follow-up shots fire on a timer, so several rounds can be airborne
at once. The cadence is controlled by a new per-item ruleset attribute:

- **`fireInterval`** (RuleItem, milliseconds, default `150`) — how long to wait between
  consecutive shots of a burst/spray. This is a largely cosmetic pacing knob; lower values make
  a weapon "spray" faster with more rounds visible simultaneously, higher values space the shots
  out. The interval is converted internally into think-cycles (the state ticks at ~60 Hz) with a
  minimum of one cycle between shots.

### Shotgun pellets as real projectiles

Shotgun spreads no longer "teleport" their extra pellets to instantly-traced impact points.
When a shotgun shot is fired, the full pellet count is launched as individual, concurrently
flying projectiles (using the concurrent projectile system above). Each pellet:

- spreads from the muzzle using the existing `shotgunSpread` / `shotgunChoke` /
  `shotgunBehaviorType` ruleset values (the spread math is unchanged from before),
- flies along its own visible trajectory, and
- resolves its own impact independently — applying damage and range-based power falloff over its
  own travelled distance through the normal projectile/explosion path.

Because each pellet is now a regular projectile, a pellet that lands on an enemy awards firing
experience like any other hit; the previous artificial per-shot experience cap (which existed
only because the old extra pellets were resolved instantly in a single synchronous burst) no
longer applies.

### Live impact recalculation

Because a projectile's trajectory and impact point are computed once when it is fired, an early
hit in a volley can destroy the wall or object that a later, still-flying round was going to hit.
Those later rounds now re-trace their path against the current map when they reach their old
impact point: if the obstacle is gone, the round keeps flying to the next real obstruction (or the
map edge) instead of detonating in mid-air on a wall that no longer exists. This also covers a
target killed by an earlier round — its corpse no longer blocks the tile. (Thrown and arcing shots
are unaffected; only straight shots recalculate.)

### Non-blocking impact explosions

Previously the entire battle froze for the duration of every impact's explosion/hit animation,
which made overlapping volleys stutter on each hit. Impact explosions from gunfire now animate
**concurrently** with the rest of the volley: the remaining rounds keep flying and the weapon
keeps firing while each hit's explosion plays out. Explosion **damage** is still applied the
instant the round lands (so the live-recalculation above sees destroyed terrain immediately); only
the animation and its end-of-animation casualty resolution run alongside continued fire. Thrown
grenades and blaster-launcher waypoint shots remain single, blocking actions.

### Burst fire mode

Adds **Burst** as a fourth firing mode alongside Snap, Auto, and Aimed (`BA_BURSTSHOT`). It is a
short, controlled volley designed to sit between snap and auto: its own accuracy, TU/energy cost,
shot count, and effective range, all loaded from dedicated ruleset keys on the weapon. Burst is
**opt-in** — a weapon only offers it when `tuBurst` (its TU cost) is set, exactly like Auto opts
in via `tuAuto`; weapons without it behave exactly as before.

- **Ruleset keys** (per weapon, parallel to the snap/auto/aimed keys):
  - `tuBurst` / `costBurst:` — TU/energy cost; **setting the TU cost enables the mode**.
  - `accuracyBurst` — burst accuracy.
  - `burstShots` — rounds per burst (default `2`).
  - `burstRange` — effective range band (default `10` tiles, between snap's 15 and auto's 7).
  - `flatBurst:` — flat-cost flag (falls back to aimed).
  - `confBurst:` — the generic action block (custom `name`/`shortName`, `ammoSlot`,
    `spendPerShot`, etc.). The mode's default display name is `STR_BURST_SHOT` ("Burst Shot").
- **Fires like auto.** Burst reuses the existing async multi-shot path: it launches its rounds
  sequentially on the shared `fireInterval` cadence (so several burst rounds can be airborne at
  once), rather than as a simultaneous shotgun-style volley.
- **Own action-menu entry & hotkey** — labeled from `confBurst.name`, bound to
  `keyBattleActionItem6` (the DX-added 6th key; `keyBattleActionItem5` is taken by Throw on
  throwable firearms); the menu shows its shot count and flags an ammo warning when the loaded
  clip holds fewer rounds than the burst needs.

