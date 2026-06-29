# OpenXcom DX — Features

This document tracks features added in OpenXcom DX on top of OXCE-Plus. Entries will be added
here as features are implemented.

## Configurable Inventory Layouts

Different armors can now grant different inventory **section sets** (slots), instead of every unit
in the game sharing one global grid. In stock OXCE the `invs:` sections are global and armor's only
inventory control is the `allowInv` on/off toggle; DX adds a per-armor layout on top.

Define named layouts with a new top-level `inventoryLayouts:` node and assign one to an armor via
`Armor.inventoryLayout`:

```yaml
inventoryLayouts:
  - type: STR_LAYOUT_LIGHT
    sections:
      - ref: STR_RIGHT_HAND      # reuse a globally-defined `invs` section by id
      - ref: STR_LEFT_HAND
      - ref: STR_BELT
      - id: STR_SATCHEL          # ...or define a section inline (same fields as an `invs` entry)
        x: 192
        y: 37
        type: 0                  # 0 = slot, 1 = hand, 2 = ground
        slots: [ [0,0], [1,0], [2,0], [0,1], [1,1], [2,1] ]
        costs: { STR_RIGHT_HAND: 8, STR_BELT: 12, STR_GROUND: 10 }

armors:
  - type: STR_HEAVY_SUIT
    inventoryLayout: STR_LAYOUT_LIGHT
```

Details and behavior:

- **Sections** are either a `ref:` to a global `invs` section (shared by id) or defined inline (the
  layout owns them; same fields as an `invs` entry). Layouts support the standard `refNode` parent
  mechanic for reuse between layouts.
- **Keying is armor-only.** Every unit always has an armor (soldiers, aliens, HWPs), so the armor's
  layout fully determines its slots. There is no `RuleSoldier`/`Unit` layout field.
- **Default behavior is unchanged.** With no `inventoryLayouts` defined (or an armor that sets none),
  the engine uses an implicit default layout synthesized from the global `invs` set, in the historical
  iteration order — so unmodified mods behave exactly as before.
- A ground section is always guaranteed (appended if a layout omits one).
- The inventory screen, item placement, quick-move (ctrl+click), start-of-mission auto-equip, and the
  alien inventory all honor the active unit's layout: hidden sections are not drawn or used, and inline
  sections are drawn, clickable, and valid auto/quick-move targets.
- **Stranding protection.** Item placement that would force an item into a section the unit's layout
  lacks (loadout templates, persistent equipment layouts saved under a different armor) instead leaves
  the item on the ground; templates show a warning (`STR_DX_TEMPLATE_SLOT_NOT_IN_LAYOUT`). Loading a
  battlescape save whose item slots became invalid because the layout/armor definition changed *since
  the save* drops those items to the unit's tile (logged), so they stay accessible.
- **Layout-aware unload.** Weapon unload only uses hand sections present in the unit's layout; when no
  off-hand is available, the ejected ammo is best-fit into another inventory slot (ctrl+click style)
  before falling back to the ground.

## Inventory Ammo-Count Badges

Weapons and ammo clips in the inventory now show their remaining rounds as a small bordered number
at the **top-right** of the item, so loadouts can be read at a glance without hovering. A weapon
shows its loaded ammo's rounds (or self-ammo charge); a clip shows its own remaining rounds;
single-shot ammo (clip size ≤ 1, e.g. a rocket) shows nothing. The number is colored by state:
**green** when full, **amber** at half-or-better, **red** below half. These colors are configurable
via three new `inventory` interface elements — `ammoFull`, `ammoMid`, `ammoLow`
(see `interfaces.rul`).

The same count badge is drawn on the right-side ammo **preview** box, which now also appears when
hovering a bare ammo clip (previously only loaded weapons previewed). The old "AMMO ROUNDS LEFT"
text was removed as redundant. (See design doc: `plans/Feature-InventoryAmmoCount.md`.)

## Edit Soldier Inventories from New Battle

The New Battle setup now lets you arrange soldier loadouts in the full inventory screen before
starting the fight. The **Inventory** button is available both from **Equip Craft → Equipment** and
from a soldier's **Soldier Info** screen (previously hidden in New Battle). Edited loadouts are saved
and carried into the battle when you start it. (The other base-management actions — Sack, transfer to
Craft, Transformations — remain unavailable in New Battle.)

## Quick Craft Stock Buttons (New Battle)

In New Battle, the **Equip Craft → Equipment** screen gains a **Fill** button next to the existing
**Unload Craft** button, for fast loadout setup:

- **Fill** instantly stocks the craft with a generous spread of every usable item (recoverable,
  non-corpse inventory items): **40** of each ammo / grenade / proximity grenade / flare, **10** of
  everything else (weapons, tools, medikits). Open the inventory afterward to distribute them.
- **Unload Craft** (unchanged) empties the craft.

Both buttons appear only in New Battle, where the loadout is a sandbox — the real geoscape craft
screen, bound by base storage and the economy, is unaffected.

## Contextual Inventory Info Panel

Hovering an item in the inventory replaces the soldier stat panel with information about that item, in
the same `STAT>VAL` style and colors as the soldier stats:

- **Granted unit stats** the item provides — one line per modified stat, flat and/or percentage
  (e.g. `FA>+5`, `RE>+10%`), each in its matching soldier stat color.
- **Directional armor bonuses** the item provides (`F>/L>/R>/B>/U>`), in the armor colors.
- Below the stats, **item-specific info**: weapon shot modes with base accuracy (`Snap>`, `Aimed>`,
  `Auto>`, `Burst>`, `Melee>`, colored like firing/melee) and medikit charges (`Heal>`/`Stim>`/`Pain>`,
  colored like health/energy/morale).
- The **weight line** shows the hovered item's own weight (in the normal weight color).

Items with none of the above keep the soldier stats; moving off the item restores the full soldier
panel. The panel reuses the expanded stat rows, so it follows the `showMoreStatsInInventoryView`
option. (See `plans/Feature-InventoryContextualInfoPanel.md`.)

## Inventory Move TU Costs in Pre-Battle Setup

The per-slot inventory move TU costs (shown on the slot labels while an item is held) now also
appear during the pre-battle equipment setup phase, not only during a mission. These are the same
rule-based costs that will apply once the battle starts, so loadouts can be planned around them;
no TUs are actually spent in the setup phase. The display still honors the existing
`oxceDisableInventoryTuCost` option — turning it off hides the costs in both phases.

## Armor Degradation

Battlescape damage types can now wear armor even on a hit that fails to penetrate, as long as the
hit was strong enough to meaningfully challenge that side's armor, and can wear extra armor on a
hit that smashes well past it. This is controlled per `damageAlter` / `RuleDamageType` via four
new knobs:

- `ToArmorBlocked` — the blocked-hit armor wear multiplier, applied only to fully blocked hits.
- `ToArmorBlockedThreshold` — the minimum fraction of the struck side's effective armor block the
  rolled damage must reach before blocked-hit wear begins.
- `ToArmorOverPen` — the over-penetration extra-wear multiplier, applied only to hits that punch
  through the struck side's armor.
- `ToArmorOverPenThreshold` — the multiple of the struck side's effective armor the rolled damage
  must exceed before over-penetration wear begins.

The existing fork behavior is preserved around it: `ToArmorPre` still applies unconditional
pre-armor wear, and `ToArmor` still applies only from post-armor penetrating damage. DX adds two
further stages: one for "nearly penetrated but was stopped" hits (`ToArmorBlocked`, defaults `0.0`
/ `0.5`) and one for "smashed clean through" hits (`ToArmorOverPen`, defaults `0.0` / `2.0`). All
defaults are no-ops, so existing mods keep their prior behavior until they opt in.

## Editable Base Damage Types

The built-in damage types (`DT_AP`, `DT_IN`, `DT_HE`, `DT_LASER`, `DT_PLASMA`, `DT_STUN`,
`DT_MELEE`, `DT_ACID`, `DT_SMOKE`, plus the ten spare slots `DT_10`..`DT_19`) can now be retuned
globally with a new top-level `damageTypes:` ruleset node, instead of overriding `damageAlter` on
every weapon. Each entry selects a slot via `ResistType` and overlays any field that per-item
`damageAlter` already understands (the `To*` multipliers, `Random*`/`Ignore*` flags,
`ArmorEffectiveness`, `RadiusEffectiveness`, `TileDamageMethod`, thresholds, etc.):

```yaml
damageTypes:
  - ResistType: 1        # DT_AP — retune armor-piercing globally
    ToArmor: 0.2
    ArmorEffectiveness: 1.1
  - ResistType: 10       # DT_10 — give a spare slot real behavior (e.g. EMP/cold/sonic)
    RandomType: 1
    ToHealth: 1.0
    ToStun: 0.5
    IgnoreDirection: true
```

Behavior and compatibility:

- All `damageTypes:` nodes are applied in a global pre-pass across **every mod's** rulesets
  **before** any `items:` load, so it does not matter which file or which mod defines the override;
  a later mod can retune a damage type that an earlier-loaded mod's (e.g. the master's) items
  inherit. Mods are swept in load order (last-wins per field). This is required because `RuleItem`
  copies the base damage type by value at load time.
- Precedence chain: built-in engine default → global `damageTypes:` edit → per-item `damageAlter`.
- `ResistType` is the lookup key only and is re-locked after load — a node cannot remap itself to a
  different slot. Out-of-range indices are reported as soft errors.
- The slot count remains fixed at 20 (`DT_NONE`..`DT_19`); adding more types is not supported.
- This is mod data, not savegame state, so there is no save-format change. Retuning base damage
  does shift balance for existing saves.
- The first cut reuses the existing `STR_DAMAGE_1x` strings for spare-slot display names; mods can
  localize those keys.

## Item Stats & Stat Modifiers

Items and armors can now apply generic stat effects using ruleset fields:

- `stats` (flat additive bonuses) on both `RuleItem` and `Armor`.
- `statModifiers` (percent deltas, where `0` means unchanged) on both `RuleItem` and `Armor`.

Runtime behavior:

- Soldier bonus preparation now applies armor `statModifiers` in addition to existing flat armor
  stat bonuses.
- Battle-unit effective stats now include equipped inventory item `stats` and `statModifiers`
  (for items in actual inventory slots), stacked with the existing soldier+armor stat baseline.
- Item `statModifiers` stack additively as percentage deltas around `0` before application.

Compatibility:

- Existing mods keep prior behavior by default; missing keys are no-op.
- Slot-level filtering (`countStats`) is intentionally deferred to the later typed-slot feature.

## Directional Armor on Items

Equipped items can now grant per-side armor to their wearer, complementing the per-side armor
already defined on `Armor`. New `RuleItem` ruleset fields:

- `frontArmor` — added to the wearer's front armor.
- `sideArmor` — added to **both** the left and right armor (matching how `Armor`'s single
  `sideArmor` expands to both sides).
- `rearArmor` — added to the wearer's rear armor.
- `underArmor` — added to the wearer's under armor.

These are distinct from the existing scalar `armor` field, which remains the item's own structural
HP / explosive resistance and is unchanged.

Runtime behavior:

- A unit's per-side max armor is `base armor (armor rule + soldier bonuses) + the directional
  armor of every equipped inventory item that occupies a slot`, recomputed whenever the inventory
  changes (alongside the effective-stat refresh).
- Accumulated per-side armor damage is tracked separately, so changing the max never refunds lost
  armor: equipping a plate while undamaged fills the new capacity, but removing and re-equipping a
  plate after taking damage restores only the plate's own armor, not the damage taken. On reload
  the saved (absolute) current armor is authoritative and the damage is derived from it.
- The fields are shown per-item in the Stats-for-Nerds screen.

Compatibility:

- Existing mods keep prior behavior by default; an item with none of the new keys is a no-op.
- No new save data: per-side max armor is derived from rules + the equipped loadout and recomputed
  on load, exactly like the existing soldier-bonus armor.

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
- Duration: advanced option **Combat log duration (seconds)** (`combatLogDuration`, default 8,
  range 1–60), under Battlescape — controls how long each entry stays before fading off. Changes
  take effect immediately, even mid-battle.
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
- **AI support.** Battlescape AI now includes Burst in both its vanilla and extended fire-mode
  selection logic, scoring it by the same accuracy/TU heuristic as the other firearm modes.
- **Own action-menu entry & hotkey** — labeled from `confBurst.name`, bound to
  `keyBattleActionItem6` (the DX-added 6th key; `keyBattleActionItem5` is taken by Throw on
  throwable firearms); the menu shows its shot count and flags an ammo warning when the loaded
  clip holds fewer rounds than the burst needs.

## Blast Radius Dropoff

- **Blast radius dropoff** (`RuleItem.blastDropoff`, default `0.0`): explosion power can now taper
  from center to edge inside the AoE radius. `0.0` keeps vanilla flat behavior, while higher
  values increasingly weight damage toward ground zero.

## Explosion VFX/Sound Radius Scaling

- Explosion presentation for area-of-effect blasts now scales from blast radius rather than
  damage power.
- Explosion sprite density/spread are driven by the resolved blast radius, so wide-radius effects
  no longer look underpowered just because their damage value is low.
- Big vs small explosion sound selection is now keyed to radius as well, keeping audio behavior
  aligned with visual blast size.

