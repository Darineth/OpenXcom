# Feature: Clairvoyance

**Status:** ✅ Implemented (Jul 2026). Test config in `bin/standard/dx-test/dx-test.rul`.
**Roadmap:** Phase 8 (Lighting & Psionics). Prereq (fog-of-war, Phase 1) is in.
**Legacy reference:** legacy source at `D:\Code\Projects\OpenXcomDX-Legacy`.
**Hard requirement:** **mod opt-in.** No ruleset node ⇒ the action does not exist and nothing changes.

## Motivation

A psi operative should be able to *look* somewhere with his mind — spend the effort, and the map opens
up around a point you can't see. It's the sensing half of psionics, next to the coercing half (panic /
mind control), and it gives psi soldiers something to do that isn't just taking an enemy's gun away.

## Audit (Jul 2026)

### OXCE has no reveal power at all

Nothing in the engine reveals an area on demand. Searching for a clairvoyance action, a reveal ability,
or any consumer of `Tile::setDiscovered` outside line-of-sight turns up only:

- `TileEngine::calculateFOV`'s own LOS walk ([TileEngine.cpp:1725](../src/Battlescape/TileEngine.cpp)),
  which marks tiles discovered as a unit actually sees them;
- the geoscape's `AlienBase::setDiscovered` (unrelated — that's the strategic layer);
- debug mode's reveal-all.

So the mechanic is genuinely absent. **`BA_CLAIRVOYANCE` does not exist** as an action type.

### What DX already has that this should reuse

Two DX systems do most of the work, and the feature is mostly a matter of *driving* them:

- **Fog of war (Phase 1).** A tile has two independent states: `setDiscovered(part)` — "this has been
  seen at some point", which is what makes it drawn at all — and `getVisible()` — "a player unit sees it
  *right now*", which is what keeps it undimmed ([Map.cpp:1293](../src/Battlescape/Map.cpp)). Revealing
  terrain is therefore exactly "mark it discovered but not visible": it appears, remembered and dimmed,
  which is precisely the right look for something seen with your mind rather than your eyes.
- **The motion-detector marker system.** DX already draws tile markers for units "scanned this turn"
  (`BattleUnit::_scannedTurn`), through walls, cleared when the player's turn ends. A clairvoyant sweep
  that reveals *units* wants exactly that behavior, so it should mark them the same way rather than
  invent a second mechanism.

### What the legacy fork did

`BA_CLAIRVOYANCE` in `PsiAttackBState.cpp`, needing no target unit:

```cpp
int psiScore = actor.psiStrength + actor.psiSkill;
if (psiScore < 100) return;                 // silently does nothing
int radius = (int)floor(0.223607 * sqrt((double)(4 * (psiScore - 100) + 5)) - 0.49);
```

giving radius 0 at 100, 1 at 110, 2 at 130, 3 at 160, 4 at 200, 5 at 250, 6 at 310. For every tile
within that radius **on the target's Z level only**, it set all three faces discovered, bumped the
visible count, and flagged any unit as a known hidden unit. Cost was `costClairvoyance` (TU + psi-amp
ammo). It was offered in the menu only when `psiStrength + psiSkill >= 100`.

Its problems, which DX should not inherit:

- **The hard-coded 100 threshold and that magic radius curve** are engine constants — a mod can't touch
  either.
- **It leaks visibility.** `changeVisibleCounts(1, 0)` with no matching decrement permanently bumps the
  tile's visible count, so revealed tiles stay *undimmed* forever — the fog never comes back.
- **Single Z level.** Revealing a floor of a building but nothing above or below it is arbitrary.
- **The AI never uses it** (`// TODO: Handle other psi actions.`).

## Design

### Ruleset (opt-in, per psi-amp)

```yaml
items:
  - type: STR_PSI_AMP
    tuClairvoyance: 30           # or costClairvoyance: { time: 30, energy: 0, mana: 0, ... }
    clairvoyance:
      enabled: true              # opt in; without this the action is never offered
      radius: 6                  # tiles, at full power
      levels: 1                  # how many Z levels above and below the target are swept (0 = just its own)
      revealUnits: true          # also mark units in the area, as the motion detector does
      minPsiScore: 0             # psiStrength + psiSkill below this can't use it at all (0 = no gate)
      scaleWithPsi: 0            # if > 0, the psi score at which the FULL radius is reached; below that
                                 # the radius scales down linearly. 0 = every psi soldier gets full radius.
```

This replaces legacy's hard-coded curve with two plain knobs a modder can reason about: a gate
(`minPsiScore`) and a linear ramp (`scaleWithPsi`). Setting neither gives a flat-radius power that any
psi soldier can use; setting both reproduces a legacy-ish "weak psychics see barely anything" feel
without a magic square root.

### Behaviour

- **No target unit needed** — you target a *tile*. (`BA_CLAIRVOYANCE` joins the psi actions; the action
  menu offers it only when the amp has the node.)
- **Terrain**: every tile within `radius` of the target, across `levels` Z levels either side, is marked
  discovered (all three faces). It is **not** marked visible — so it shows up *dimmed*, as remembered
  terrain. That is both correct (you aren't looking at it) and free: it's just fog-of-war doing its job,
  and it means the reveal is permanent map knowledge, exactly like walking past it would be.
- **Units** (`revealUnits`): any unit in the area is marked scanned-this-turn, so DX's existing
  motion-detector markers show it through walls until the player's turn ends. Deliberately *not* a
  permanent spotting — clairvoyance tells you something is *there now*, not who or what it is forever.
- **Cost**: `tuClairvoyance` / `costClairvoyance`, via the standard `RuleItemUseCostRule::loadCost`
  machinery, so it supports TU/energy/mana/morale like any other action.

### Not in scope

- **Psi-amp ammo** — legacy charged orbs per cast. That's its own roadmap item (Psi-Amp Ammo Mechanics),
  and once it lands, clairvoyance picks it up for free through the shared cost block.
- **AI use** — the AI has never used clairvoyance and gains little from it (it has its own spotting
  rules). Left out; revisit if enemy psi units should scout.

## Decisions (Jul 2026)

1. **Units are marked, not spotted.** They get the motion-detector treatment (a marker through walls,
   cleared at end of turn), reusing machinery DX already has. Fully spotting them would let a psi soldier
   hand the squad free shots at enemies nobody can see, which is too strong.
2. **Targeting is limited by the amp's range**, like the other psi actions — not free-aim anywhere on the
   map as legacy allowed.
