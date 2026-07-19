# Feature: Stealth / Cloaking Armor

**Status:** ✅ Implemented (Jul 2026). Test content in `bin/standard/dx-test/dx-test.rul`: a **Stealth
Suit** (a buyable Personal Armor reskin) carrying the dynamic cloak, plus the flying suit as a
static-camouflage reference (always-on camo, always ghosted, never breaks).
**Roadmap:** Phase 8 (Lighting & Psionics).
**Legacy reference:** `Legacy-DX-Features.md` §15; the legacy DX fork's source.

## Motivation

A soldier in stealth armor should be hard to see, should *look* cloaked, and should lose the cloak
when they do something loud. Legacy DX had this as one component (`EC_STEALTH`) of its Effects
framework — the framework DX
[dropped](../DX-Roadmap.md) in the Phase 8 replan — so the question is which parts of it are real
deltas over modern OXCE, and which are already native.

## Audit (Jul 2026)

### The spotting math is already in OXCE, and is strictly better than legacy's

Legacy `EC_STEALTH` was a single block in `TileEngine::visible` (legacy `TileEngine.cpp:1121-1131`):
the target's stealth `magnitude` scaled the observer's max sight range,
`visibleDistanceMaxVoxel = visibleDistanceMaxVoxel * (100 - magnitude) / 100`, and `magnitude >= 100`
set it to `0` — invisible at *every* range, including adjacent. It was applied after the day/dark
clamps, was bypassed by `psiVision`, and never hid a unit from its own faction.

OXCE covers this with `Armor.camouflageAtDay` / `camouflageAtDark` (target-side) and
`antiCamouflageAtDay` / `antiCamouflageAtDark` (observer-side), consumed in
[BattleUnit::getMaxViewDistance](../src/Savegame/BattleUnit.cpp#L5149):

```
result = base                       // the observer's own max view distance (day or dark)
if (camouflage > 0) result = camouflage;   // positive = absolute cap, in tiles
else                result += camouflage;  // negative = relative reduction
if (result < 1) result = 1;                // floor: adjacent is always visible
result += antiCamouflage;                  // observer claws range back...
if (result > base) result = base;          // ...but never past its own base range
```

Compared to legacy this adds: independent day/dark values, an observer-side counter stat, and a
1-tile floor (legacy's `>= 100` allowed literal total invisibility, which is a worse gameplay
primitive). It is consumed by the one `TileEngine::visible()` funnel
([TileEngine.cpp:1927](src/Battlescape/TileEngine.cpp#L1927)), so **the AI, reaction fire, overwatch,
FOV, and the visible-unit buttons all already honor it**. A burning unit loses its camouflage
([TileEngine.cpp:1805](src/Battlescape/TileEngine.cpp#L1805)).

Also native and adjacent to this cluster: `psiVision` / `psiCamouflage` (sense units through walls,
with its own cap/reduction pair), `heatVision` (see through smoke), and the `visibilityUnit` script
hook (armor-level; can only *veto* an already-passing LOS check, and its second return value — a
"visibility mode" tag — is computed and then **discarded** by the caller).

**Conclusion: a static stealth armor needs zero engine work in DX today — it is ruleset content.**
The DX deltas are the two things below.

### Delta 1 — dynamic cloak (break on action)

OXCE camouflage is a *constant on the armor*. Legacy stealth was a **state**: re-applied at the start
of every turn and cancelled by triggers the ruleset listed (`cancelTriggers: [2, 4, 10, 11]` =
walk, sprint, activate-item, attack). Notably **sneak movement and turning did not break it**, so the
intended play pattern was: creep while cloaked, and the moment you fire you are exposed until your
next turn.

This is the interesting half of the feature, it is absent from OXCE, and it composes with work DX has
already landed — sneak/sprint movement modes, the movement-mode evasion split, and the sneak light
gate (`sneakDefaults: { maxLight }`, which already refuses to let a glowing unit creep).

Legacy also hard-countered stealth with light: a `STR_HEADLAMP` item carried an `EC_PREVENT_EFFECT`
component that blocked and cancelled the stealth effect class. DX's equivalent already half-exists in
the light-equipment feature (a lit unit can't sneak), so the cloak's light interaction should be
expressed the same way rather than as a new mechanism.

### Delta 2 — translucent rendering (genuinely absent)

There is **no translucency of any kind for units** in the engine. The battlescape is 8-bit paletted;
there is no alpha channel; [Map::drawUnit](src/Battlescape/Map.cpp#L801) is a binary
`if (!bu->getVisible()) return;` gate; `BattleUnit` has only a `bool _visible`. The one real blend
mechanism — the `transparencyLUTs` ruleset node and `Mod::getLUTs()` — is consumed **only** by vapor/
smoke particles ([Map.cpp:1578](src/Battlescape/Map.cpp#L1578)), never by units.

Legacy's shader (`RecolorStealth`, legacy `ShaderDraw.h:398-418`) was a **50% scanline cull**: on odd
raster rows it wrote palette index `0`; on even rows it did the normal armor recolor. That reads as
translucency only when the destination is a layer later blitted with index-0 transparency — true of
the inventory paperdoll surface, **not** true of the composited battlescape map surface, where
writing 0 punches holes through the terrain behind the soldier. Which is exactly why, in the legacy
tree, the effect is **live only on the inventory paperdoll**
(legacy `InventoryState.cpp:668`) and the battlescape version
(`UnitSprite::drawRecolored`) is **commented out**, with a `// TODO: Add stealth effect to this
function` left behind in `getRecolorScript`.

The correct battlescape version must **leave the destination pixel untouched** on culled pixels
rather than zero it. DX can do that: the unit blit already runs through
`ScriptWorkerBlit::executeBlit` ([Script.cpp:536-605](src/Engine/Script.cpp#L536-L605)), which passes
the destination pixel into the shader and treats a `0` result as "keep the background". DX also
already uses checkerboard dithering as its translucency idiom (the Pathfinding2 marker sheet's
dithered row, [Map.cpp:101](src/Battlescape/Map.cpp#L101)).

Options for the look, cheapest first:

1. **Checkerboard dither** (`(x + y) % 2`) — keep the background on culled pixels. Finer and less
   strobe-y than legacy's full-scanline cull, and cheap. Reads as a 50% ghost.
2. **Dither + tint via the existing transparency LUT** — culled pixels keep the background; drawn
   pixels are pushed through a LUT for a shimmer/tint. Reuses `Mod::getLUTs()`, no new ruleset
   surface if we pick a LUT index by convention (or add one small armor field).
3. **Shade-only ghost** (draw the unit uniformly darkened, no cull) — the abandoned
   `//const int stealthShade = 8;` idea in legacy `Map.cpp:347`. Cheapest, but reads as "in shadow",
   not "cloaked".

Recommendation: **(1), with (2) as a follow-up** if it looks flat. Apply to the body **and the held
item sprite** (legacy never did — its item blit didn't route through the recolor helper, so cloaked
soldiers carried a fully solid rifle).

Who sees the ghost render:
- **Your own cloaked units** — always drawn (you always see your own side); the ghost is the *feedback*
  that the cloak is currently up. This is the primary value of the render, and it's what makes the
  break-on-action rule legible.
- **Enemy units that are camouflaged but currently spotted** — arguably should also render ghosted,
  as a cue for "this one is hard to track". Optional; needs a decision (see below).
- The inventory paperdoll, as legacy did (cheap, and the same shader).

## Decisions (Jul 2026)

1. **Dynamic cloak, opt-in per armor.** The break-on-action behavior is the feature — but it is a
   *moddable armor option*, not a global rule. An armor with plain `camouflageAtDay/AtDark` and no
   cloak node keeps stock OXCE behavior (always-on camouflage); an armor that opts in gets the
   dynamic cloak.
2. **The ghost render applies to any unit with active camouflage** — your own cloaked soldiers *and*
   camouflaged enemies while they are spotted. It is a property of "is this unit currently hard to
   see", not of whose side it is on. A static-camo armor therefore also renders ghosted, with no
   extra config.
3. **No true-invisibility tier.** OXCE's 1-tile visibility floor stays; a cloak can push spot range
   very low but an adjacent unit is always seen. Legacy's `magnitude >= 100` (unspottable even when
   adjacent) is deliberately not reproduced.

## Design

### Ruleset

The armor's existing OXCE camouflage fields are the *cloaked* values. A new DX `cloak:` node makes
them dynamic:

```yaml
armors:
  - type: STR_STEALTH_SUIT_UC
    camouflageAtDay: 8       # existing OXCE fields - the values that apply while the cloak is up
    camouflageAtDark: 4
    cloak:
      dynamic: true                            # opt in; without this node the camo is always on (stock)
      breaksOn: [walk, run, attack, useItem]   # the default set; `sneak` and `turn` are also accepted
```

While the cloak is up, the unit's camouflage is the armor's; while broken, it is `0`. All of the
spotting math stays native — DX only decides *when the armor's camouflage is active*.

### Engine

- **`BattleUnit` cloak state** (`_cloakBroken`), in the same spirit as `_personalLightOn` and the
  overwatch state: save-persisted, reset at the start of the unit's turn. Effective camouflage is
  read through new `BattleUnit::getCamouflageAtDay/Dark()` accessors, which
  `getMaxViewDistanceAtDay/Dark()` use in place of the raw `armor->getCamouflage*()` calls — so every
  existing consumer (AI, reaction fire, overwatch, FOV, visible-unit buttons) follows automatically.
- **Break triggers** ride on a shared **unit-action report**, `BattlescapeGame::unitActed(unit, action,
  reaction)`, rather than a cloak-specific hook. The battle-action states (`UnitWalkBState` by movement
  mode, `ProjectileFlyBState`, `MeleeAttackBState`, `PsiAttackBState`, the turn order, the item-use
  paths) report *what the unit did*; `unitActed` owns the policy of who reacts. **Overwatch was folded
  into the same hook** — it had grown its own copy of the same checks at those same sites. A future
  consumer of "the unit acted" is a few lines in `unitActed`, not another sweep of the action states.
- Breaking or restoring the cloak **recalculates FOV and invalidates the map**, so units pop in/out
  immediately (legacy forced a `calculateLighting(LL_UNITS)` for the same reason).
- **Light interaction** reuses the existing sneak light gate rather than a new mechanism: a unit lit
  above `sneakDefaults.maxLight` already can't creep, which is the same "you're glowing, you're not
  sneaking" rule (legacy expressed this as a headlamp `EC_PREVENT_EFFECT`).

### Render

- New `CurrentPixel` shader argument in `ShaderDraw.h` (ported from legacy, but tracking
  **destination-surface** coordinates so the pattern is stable across a unit's separately-blitted
  body parts), plus a `ghost` flag on `ScriptWorkerBlit::executeBlit`.
- Ghost = **checkerboard dither**: on culled pixels the shader *leaves the destination pixel alone*
  (rather than writing index 0 as legacy did, which is why legacy's map path punched holes in the
  terrain and had to be commented out). Applies to the body **and the held item sprite** — legacy
  never stealthed the item, so cloaked soldiers carried a solid rifle.
