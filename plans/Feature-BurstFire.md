# Feature: Burst Fire Mode (`BA_BURSTSHOT`)

**Status:** Implemented (Phase 3 — Combat Infrastructure).

## Motivation

X-COM weapons historically offer only Snap / Auto / Aimed. DX adds **Burst** as a fourth firing
mode — a short, controlled volley that sits between snap and auto: more rounds than a snap shot,
but tighter/cheaper than a full auto burst. This gives modders a middle gear for firing cadence
and lets specific weapons (e.g. an assault rifle) expose a distinct 3-round burst.

## OXCE / OXCE-Plus audit

Base OpenXcom and OXCE/OXCE-Plus expose exactly three firearm fire modes, encoded as
`BA_SNAPSHOT` / `BA_AUTOSHOT` / `BA_AIMEDSHOT` in `enum BattleActionType` (in DX this enum lives in
`src/Mod/RuleItem.h`). Each mode maps to a `RuleItemAction` config block on `RuleItem`
(`_confSnap` / `_confAuto` / `_confAimed`, plus `_confMelee`) and is cached per-weapon on
`BattleItem` (`_confSnap` / `_confAuto` / `_confAimedOrLaunch` / `_confMelee`), surfaced through
`BattleItem::getActionConf(BattleActionType)`. There is **no** burst mode upstream — this is a true
DX delta. (A historic DX-lineage fork had a `BA_BURSTSHOT`; see `Legacy-DX-Features.md`. This is a
clean re-implementation on top of the current async multi-shot infrastructure.)

The async multi-shot plumbing this feature relies on already exists in DX:

- **Sequential timed firing** via `fireInterval` (see `Feature-AsyncProjectileSystem.md`):
  `ProjectileFlyBState` schedules follow-up rounds on a timer using `autoShotCounter` and
  `BattleItem::haveNextShotsForAction(type, shotCount)`, which reads `getActionConf(type)->shots`.
- **Concurrent projectiles / explosions** so multiple burst rounds can be airborne at once.

Because of that, burst needed almost no new mechanics — just a new mode that routes through the
same `getActionConf`-driven path with its own config block.

## Implementation

Burst is modeled exactly like auto: a `RuleItemAction` config block with its own accuracy / cost /
flat / shots / range, opt-in via `tuBurst` (its TU cost).

### Enum
- `src/Mod/RuleItem.h`: appended `BA_BURSTSHOT = 20` to `enum BattleActionType` (appended at the
  end to keep existing serialized `BattleActionType` values stable for save compatibility).

### RuleItem (`src/Mod/RuleItem.h` / `.cpp`)
- Added `_confBurst` to the `RuleItemAction` member list.
- Constructor defaults: `_confBurst.range = 10`, `_confBurst.shots = 2`,
  `_confBurst.name = "STR_BURST_SHOT"`.
- `load()`: reads `accuracyBurst`, `burstRange`, `burstShots`, the `costBurst`/`flatBurst` cost
  blocks (`tuBurst`, etc.), and the generic `confBurst:` action block; added `&_confBurst` to the
  afterLoad ammo-slot validation loop.
- Getters: `getConfigBurst()`, `getAccuracyBurst()`, `getCostBurst()` (falls back to aimed cost),
  `getFlatBurst()` (falls back to aimed flat).

### BattleItem (`src/Savegame/BattleItem.h` / `.cpp`)
- Added the cached `_confBurst` pointer, assigned from `getConfigBurst()` in the firearm/melee
  branch of the constructor, and included in the per-slot `used |=` ammo-visibility loop.
- `getActionConf()`: `case BA_BURSTSHOT: return _confBurst;`. This single mapping makes
  `needsAmmoForAction` / `getAmmoForAction` / `getArcingShot` / `haveNextShotsForAction`
  all work for burst automatically.

### BattleUnit (`src/Savegame/BattleUnit.cpp`)
- `getFiringAccuracy()`: added a `BA_BURSTSHOT` branch using `getAccuracyBurst()`.
- `getActionTUs` cost/flat switch: added `case BA_BURSTSHOT` using `getFlatBurst()` /
  `getCostBurst()`.

### Battle flow
- `src/Battlescape/ProjectileFlyBState.cpp`: added `BA_BURSTSHOT` to the in-range check switch and
  to the shot-type name switch (uses `getConfigBurst()->name`). Shot count and cadence come for
  free from `haveNextShotsForAction` + `fireInterval`.
- `src/Battlescape/BattlescapeGame.cpp`: added `BA_BURSTSHOT` to the `primaryAction` dispatch
  condition so the action pushes a `ProjectileFlyBState` like the other fire modes.

### AI (`src/Battlescape/AIModule.cpp`)
- Added burst to both AI fire-mode choosers: the vanilla distance-based fallback and the extended
  accuracy-per-TU scoring path.
- `scoreFiringMode()` now counts `getConfigBurst()->shots` so burst is evaluated like the other
  multi-shot firearm modes.
- Added `BA_BURSTSHOT` to the committed-shot kneel behavior and the nearby weapon-power comparison
  list so the AI treats burst as a normal firearm action once selected.

### Action menu (`src/Battlescape/ActionMenuState.cpp`)
- Added a burst entry in the firearm block, gated on `getCostBurst().Time > 0` (opt-in via
  `tuBurst`, exactly like auto's `tuAuto` gate), bound to `keyBattleActionItem6` (the DX-added
  6th key; `keyBattleActionItem5` is already used by Throw on throwable firearms).
- Added `BA_BURSTSHOT` to the accuracy-display, ammo-lookup, and affordability conditions so the
  menu shows accuracy / shot count / ammo warnings for burst.

### Localization
- `bin/common/Language/DX/en-US.yml`: added `STR_BURST_SHOT: "Burst Shot"`.

### Test content
- `bin/standard/dx-test/dx-test.rul`: the test rifle now enables burst (`accuracyBurst: 55`,
  `burstShots: 3`, `tuBurst: 33`, `burstRange: 12`) alongside its existing fast 6-round auto.

## Notes / decisions

- **Opt-in gate.** Burst opts in via `tuBurst` exactly like auto opts in via `tuAuto`. The cost's
  `.Time` sub-value defaults to `0` (a real, non-null value), so `getCostBurst()` does **not**
  inherit the aimed TU — only the other cost fields fall back to aimed. The action menu therefore
  gates the entry purely on `getCostBurst().Time > 0`, matching the standard auto/snap behavior
  (no separate accuracy gate; accuracy will be revamped later).
- **TU reservation.** Burst is intentionally not added to the snap/auto/aimed TU-reservation
  system; it is a manual fire mode only.
- **AI.** The AI's explicit attack-option list (`AIModule`) was left unchanged, so the AI does not
  pick burst on its own. Can be revisited if a designer wants AI burst usage.
- **Ufopaedia / Stats-for-nerds.** Burst is not yet surfaced in the article/stat screens; this is
  cosmetic and can be added later.
