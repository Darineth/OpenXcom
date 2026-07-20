# Feature: AI Fixes — TU reserve, reaction fire, counter-mind-control

**Status:** ✅ **Implemented (Jul 2026)** — builds clean; pending in-game validation (§6).
Both options ship **default off**, enabled in `bin/standard/dx-test/dx-test.rul` for testing.
**Roadmap:** [DX-Roadmap.md](../DX-Roadmap.md) Phase 9: AI.

This covers the Phase 9 "AI fixes" item: make the AI use **normal TU-reserve logic** instead of
custom percentages, fix the reaction-fire consequence of that bug, and settle the
counter-mind-control sub-item.

It also records the audit that **closes the Phase 9 "per-weapon AI targeting" item as superseded**
— see [§4](#4-per-weapon-ai-targeting--closed-as-superseded).

---

## 1. Motivation

An alien that walks into the open and then stands there with 3 TUs left, unable to shoot and unable
to react, is the single most immersion-breaking AI behavior in the tactical layer. It is not a
tuning problem — it is two concrete bugs in the reserve plumbing, both inherited verbatim from
base OpenXcom.

---

## 2. Audit — what ships today (Jul 2026)

### 2.1 The hostile reserve branch is a flat percentage of *base* TU

[`BattlescapeGame::checkReservedTU`](../src/Battlescape/BattlescapeGame.cpp#L1547) has a
hostile-only early-return:

```cpp
// BattlescapeGame.cpp:1559-1577
if (_save->getSide() == FACTION_HOSTILE && !_debugPlay) // aliens reserve TUs as a percentage...
{
    AIModule *ai = bu->getAIModule();
    if (ai) cost.type = ai->getReserveMode();
    cost.updateTU();
    cost.Energy += energy;
    cost.Time = tu; // override original
    switch (cost.type)
    {
    case BA_SNAPSHOT:  cost.Time += (bu->getBaseStats()->tu / 3);     break; // 33%
    case BA_AUTOSHOT:  cost.Time += ((bu->getBaseStats()->tu / 5)*2); break; // 40%
    case BA_AIMEDSHOT: cost.Time += (bu->getBaseStats()->tu / 2);     break; // 50%
    default: break;
    }
    return cost.Time <= 0 || cost.haveTU();
}
```

Three defects, in order of severity:

1. **It reserves a fraction of `getBaseStats()->tu`** — the unit's *maximum* TU — not the cost of
   the shot it intends to take. A wounded/encumbered unit reserves the same absolute amount as a
   fresh one, and a cheap pistol reserves as much as a heavy cannon.
2. **It early-returns**, skipping the player fallback chain immediately below
   ([`:1579-1606`](../src/Battlescape/BattlescapeGame.cpp#L1579)): autoshot → snapshot → aimed →
   kneel → none. So a weapon with a **non-standard shot config** (no snapshot, e.g. a launcher or
   a burst-only weapon) still "reserves" 33% of max TU for a shot it can never fire. This is the
   legacy commit's *"reserved TU fallback logic to handle weapons with non-standard shot
   configurations"*.
3. `cost.Time = tu` discards the `cost.updateTU()` result entirely, so the weapon is consulted only
   to be thrown away.

### 2.2 `_reserve` is only ever set during patrol

[`AIModule::think`](../src/Battlescape/AIModule.cpp#L604) resets `_reserve = BA_NONE` and then sets
it in exactly **one** branch — `AI_PATROL`
([`:621-639`](../src/Battlescape/AIModule.cpp#L621)), keyed on aggression
(0 → aimed, 1 → auto, 2 → snap), and only for `BT_FIREARM`.

`AI_COMBAT`, `AI_AMBUSH` and `AI_ESCAPE` all leave `_reserve = BA_NONE`. `getReserveMode()`
([`:3056`](../src/Battlescape/AIModule.cpp#L3056)) just returns the field. So **while actually
fighting, the AI reserves nothing** and `UnitWalkBState`'s per-step gate
([UnitWalkBState.cpp:328](../src/Battlescape/UnitWalkBState.cpp#L328)) happily lets it walk to
~0 TU.

### 2.3 Reaction fire — the "one in a million" bug is a *symptom*

[`TileEngine::determineReactionType`](../src/Battlescape/TileEngine.cpp#L2995) checks only the
**actual** shot cost — `BattleActionCost(...).haveTU()` at
[`:3086`](../src/Battlescape/TileEngine.cpp#L3086) and
[`:3098`](../src/Battlescape/TileEngine.cpp#L3098). There is no reserve check anywhere in the
reaction path, and that is correct.

The bug is upstream: a unit that walked itself dry under §2.2 fails `haveTU()` and silently never
reacts. Hence the legacy fork's framing of these as one change — **fixing §2.1 + §2.2 fixes
reaction fire**, with no edit to the reaction path itself.

*(An earlier draft of this doc claimed a second residual — that `determineReactionType` prefers
melee over firearms at any range. **That was wrong and has been retracted.** The melee branch is
gated on `validMeleeRange(...)` at [`:3084`](../src/Battlescape/TileEngine.cpp#L3084), so melee only
wins when the target is actually adjacent; otherwise the loop falls through to the snapshot check
and on to the next candidate weapon. The candidate ordering at
[`:3030-3039`](../src/Battlescape/TileEngine.cpp#L3030) only breaks the tie when both are viable.
Nothing to fix.)*

### 2.4 Counter-mind-control — **already shipped in DX**

Contrary to the roadmap line, this is done. [`AIModule::psiAction`](../src/Battlescape/AIModule.cpp#L2738)
already filters targets as
`(bu->getOriginalFaction() != _unit->getFaction() || bu->isMindControlled())`
([`:2782-2786`](../src/Battlescape/AIModule.cpp#L2782)) — the in-code comment explicitly notes that
stock filters on *original* faction, "which is exactly why the legacy fork's counter-control could
never actually be reached by the AI". It adds `weightToAttackMe += 80` for freeing one of its own
([`:2824-2829`](../src/Battlescape/AIModule.cpp#L2824)) and refuses to panic a puppet
([`:2811-2816`](../src/Battlescape/AIModule.cpp#L2811)).

Engine side, [`TileEngine`](../src/Battlescape/TileEngine.cpp#L5096) contests the **controller's**
psi defence rather than the puppet's, with `counterControlFree` handling at
[`:5177-5183`](../src/Battlescape/TileEngine.cpp#L5177). Shipped as part of
[Feature-ChanneledMindControl.md](Feature-ChanneledMindControl.md); documented in `DX-Features.md`.

**Verdict:** tick the sub-item. The only residue is optional polish (below).

### 2.5 Existing ruleset surface

`docs/Ruleset-AI.md` has **no reserve-related keys**. `Extended.txt:770-776` shows the global `ai:`
node is only the `useDelay*` family. There is no OXCE option in this area — this is a true delta.

---

## 3. Design

### 3.1 Bypass the hostile percentage branch when `normalTUReserve` is on

Since the option defaults off (§3.3), the stock percentage path at
[BattlescapeGame.cpp:1559-1577](../src/Battlescape/BattlescapeGame.cpp#L1559) **stays**. Gate it so
that with `normalTUReserve: true` hostiles fall through to the same path the player uses. The AI's
chosen reserve mode still has to reach `cost`, so hoist the `getReserveMode()` lookup out of the
branch:

```cpp
if (_save->getSide() == FACTION_HOSTILE && !_debugPlay)
{
    if (AIModule *ai = bu->getAIModule())
    {
        cost.type = ai->getReserveMode();
    }
    if (!_parentState->getGame()->getMod()->getAINormalTUReserve())
    {
        // ...stock percentage switch, unchanged, plus the new BA_BURSTSHOT case (§4.1)
    }
    // else fall through to the shared player logic below
}
cost.Energy += energy;
// ...shared updateTU() + auto→burst→snap→aimed→kneel→none chain
```

This buys all three fixes at once: real shot costs, weapon-aware, and the non-standard-shot-config
fallback. Note the shared chain already contains the `cost.Time == 0` guards that make a
burst-only or launcher-only weapon degrade gracefully instead of reserving a phantom third of max TU.

**Caution — shared function.** `checkReservedTU` is also on the player path via
[Pathfinding.cpp:1254-1258](../src/Battlescape/Pathfinding.cpp#L1254) / `:1338-1341`, which
temporarily forces `BA_AUTOSHOT` and restores `BA_NONE` around path preview. The edit must stay
inside the hostile branch or it will change player path-preview colors. Verify preview colors are
unchanged before considering this done.

### 3.2 Set `_reserve` in the combat AI modes

Extend the reserve selection in [`AIModule::think`](../src/Battlescape/AIModule.cpp#L604) beyond
`AI_PATROL`. Factor the aggression switch out of the `AI_PATROL` case into a small helper
(`AIModule::pickReserveMode()`), and call it from `AI_PATROL`, `AI_COMBAT` and `AI_AMBUSH`.

Per-mode intent:

| AI mode | Reserve | Rationale |
|---|---|---|
| `AI_PATROL` | aggression-keyed (unchanged) | Existing behavior; a patrolling unit should keep a shot in hand. |
| `AI_COMBAT` | aggression-keyed, **but not when the pending action is itself an attack** | Reserving on top of the shot it is about to take would deadlock it. Only reserve when `_attackAction.type == BA_WALK` (the find-fire-point / reposition case). |
| `AI_AMBUSH` | aggression-keyed | The whole point of an ambush is to hold a shot. |
| `AI_ESCAPE` | `BA_NONE` (unchanged) | A fleeing unit should spend everything on distance. |

Also drop the `BT_FIREARM`-only gate to include melee weapons — a charging unit that arrives with
no TU to swing is the same bug.

### 3.3 Gating

Behavior-changing for every existing mod, so gate it and default **off** — stock behavior is
preserved on upgrade, and the fix is opted into explicitly. **Decision (Jul 2026):** default off,
enabled in `bin/standard/dx-test/dx-test.rul` for testing.

New `ai:` node keys, documented in `docs/Ruleset-AI.md` under a new "TU reserve" section:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `normalTUReserve` | bool | `false` | **[DX]** AI reserves the actual cost of its intended shot (same logic as the player, with the auto→burst→snap→aimed→kneel fallback). Default `false` = stock behavior: reserve a flat percentage of the unit's *maximum* TU. |
| `combatTUReserve` | bool | `false` | **[DX]** AI also reserves TUs while in combat/ambush modes, not only while patrolling. Default `false` = stock patrol-only behavior. |

Because the default is off, **the stock percentage branch must be kept, not deleted** — §3.1
becomes a branch on `normalTUReserve` rather than a removal. That in turn means the burst case in
the percentage switch (§4.1) is live code, not dead, and does need writing.

Add to `dx-test.rul` with the usual commented block explaining what to watch for:

```yaml
ai:
  normalTUReserve: true
  combatTUReserve: true
```

Parsed in `Mod::loadFile` alongside the other `ai:` keys ([Mod.cpp:3370](../src/Mod/Mod.cpp#L3370)
area), accessors on `Mod`, consumed in `checkReservedTU` and `AIModule::think`.

### 3.4 Optional sub-fixes (decide during implementation)

- **Reaction weapon choice** — ~~melee preferred over firearms at any range~~ **retracted, not a
  bug.** See the note in §2.3: `validMeleeRange` already gates it to adjacent targets.
- **Target-the-controller weighting** — weighting a mind-controller up as a conventional-weapon
  target, since killing it breaks the link. **Decision (Jul 2026): deferred.** Revisit only if
  counter-control plays badly without it.

---

## 4. `BA_BURSTSHOT` gap sweep

Burst is a **DX-added** fourth fire mode, so every stock code path that enumerates
snap/auto/aimed predates it. A sweep found a long tail of constructs that never learned about
burst. The reserve fallback chain in §3.1 is one of them, which is why this lands in the same
feature — fixing the reserve ladder without adding a burst rung would just move the bug.

**The established idiom to copy** is [`BattleItem::getActionConf`](../src/Savegame/BattleItem.cpp#L905)
(`case BA_BURSTSHOT: return _confBurst;`), and
[`BattleUnit::getActionTUs`](../src/Savegame/BattleUnit.cpp#L2388)
(`getFlatBurst()`/`getCostBurst()`). `RuleItem::getDualFireMode`
([RuleItem.cpp:2621](../src/Mod/RuleItem.cpp#L2621)) establishes the DX preference order
**Auto → Burst → Snap → Aimed**; reuse it rather than inventing a new one.

The general failure signature: a **burst-only weapon** (burst configured, no snap/auto) is
invisible to these paths and silently does nothing.

### 4.1 In scope — the reserve/reaction paths this feature already touches

| Site | Gap | Fix |
|---|---|---|
| [BattlescapeGame.cpp:1569-1575](../src/Battlescape/BattlescapeGame.cpp#L1569) | Hostile percentage switch has no `BA_BURSTSHOT` — falls to `default` and reserves **0**. | **Live code** — the stock branch is retained as the default (§3.3). Add `case BA_BURSTSHOT:` between snap and auto: `(bu->getBaseStats()->tu * 3) / 8` ≈ 37%. |
| [BattlescapeGame.cpp:1581-1604](../src/Battlescape/BattlescapeGame.cpp#L1581) | Fallback ladder is auto→snap→aimed→kneel→none; **burst is not a rung**. | Make it auto→**burst**→snap→aimed→kneel→none, matching `getDualFireMode`. This is the core of the "non-standard shot configurations" fix. |
| [BattlescapeGame.cpp:1631-1637](../src/Battlescape/BattlescapeGame.cpp#L1631) | Reserve-failure warning switch has no burst case → **silent** failure. | Add `STR_TIME_UNITS_RESERVED_FOR_BURST_SHOT`. **New string** → `bin/common/Language/DX/en-US.yml` per CLAUDE.md. |
| [AIModule.cpp:625-638](../src/Battlescape/AIModule.cpp#L625) | Aggression→reserve map (0/1/2 → aimed/auto/snap) can never yield burst, so `getReserveMode()` never returns it. | In the §3.2 `pickReserveMode()` helper, degrade to burst when the chosen mode is unconfigured — otherwise `_reserve = BA_AUTOSHOT` on a burst-only weapon reserves nothing. |
| [TileEngine.cpp:3092-3102](../src/Battlescape/TileEngine.cpp#L3092) | `determineReactionType` offers only `BA_HIT` then `BA_SNAPSHOT`. A burst-only weapon **can never reaction fire**. | Fall back to `BA_BURSTSHOT` when snap is unconfigured. Note overwatch already parses `overwatchShot: burst` ([RuleItem.cpp:545](../src/Mod/RuleItem.cpp#L545)) — reaction and overwatch **diverge** today, which is its own inconsistency. |
| [TileEngine.cpp:2828](../src/Battlescape/TileEngine.cpp#L2828) | LOF probe hardcodes `falseAction.type = BA_SNAPSHOT`. | Same burst-only blind spot; follow the reaction fix. |
| [Pathfinding.cpp:1254-1258](../src/Battlescape/Pathfinding.cpp#L1254) | Preview forces `BA_AUTOSHOT`; on a burst-only weapon that costs 0 and mis-colours the preview. | Fall back to burst. Overlaps the §3.1 regression risk — test together. |

### 4.2 Adjacent correctness bugs — same root cause, worth fixing here

These are outside the reserve path but are the same one-line omission, and leaving them makes
burst-only weapons feel broken:

- [BattleItem.cpp:851-853](../src/Savegame/BattleItem.cpp#L851) — the ammo check tests
  aimed ‖ auto ‖ snap. **A burst-only weapon reports as having no usable ammo.** *(verified)*
  Highest-severity item in the sweep.
- [UnitPanicBState.cpp:73-96](../src/Battlescape/UnitPanicBState.cpp#L73) — berserk shot selection
  is auto→snap→aimed, so a berserking unit with a burst-only weapon cannot shoot.
- [BattleUnit.cpp:2019](../src/Savegame/BattleUnit.cpp#L2019) — "was hit by" AI tracking ignores
  burst, so being *burst-fired at* does not reset `turnsSinceSpotted` or call `setWasHitBy()`.
  Directly AI-relevant: the AI does not notice it is being shot.
- [AIModule.cpp:657-659](../src/Battlescape/AIModule.cpp#L657) — the find-fire-point check tests
  only `BattleActionCost(BA_SNAPSHOT, ...)`; same at `:493-498`, `:810-817`.
- **Spray targeting** ([BattlescapeGame.cpp:1814](../src/Battlescape/BattlescapeGame.cpp#L1814),
  `:1941`, `:1976`) — `sprayWaypoints` is gated on `BA_AUTOSHOT` only. **Decision (Jul 2026):
  include burst.** This is *upstream OXCE* code that predates DX's burst mode, so the omission is
  an oversight rather than a design line — burst is a multi-round volley and spraying it down a
  waypoint line is exactly as sensible as for auto. Since this changes pre-existing upstream
  behavior it earns a `DX-OXCE-Fixes.md` entry.
- [AIModule.cpp:2645-2648](../src/Battlescape/AIModule.cpp#L2645) — `extendedFireModeChoice`
  applies the aggression bonus to `BA_AUTOSHOT` only. **Decision (Jul 2026): burst gets the bonus,
  scaled by shot count.** Auto keeps its current full-strength bonus as the reference point, and
  burst receives it in proportion to how much of an auto volley it represents — so a 3-round burst
  next to a 8-round auto gets ~⅜ of the bonus. Concretely, scale by
  `confBurst.shots / confAuto.shots`, clamped to 1.0, falling back to full bonus when the weapon
  has no auto mode to compare against (a burst-only weapon *is* its spray mode).

### 4.3 Out of scope — log separately

Real gaps, but UI/scripting rather than AI. They should not ride along in an AI-fix commit:

- **No burst reserve button or hotkey — deferred to a future battle-UI revamp.**
  [BattlescapeState.cpp:1985-1992](../src/Battlescape/BattlescapeState.cpp#L1985) has only
  None/Snap/Aimed/Auto, and `init()`'s reverse mapping
  ([`:839-853`](../src/Battlescape/BattlescapeState.cpp#L839)) has no burst case — a burst reserve
  restored from a save silently displays as "None". No `keyBattleReserveBurst` in
  [Options.cpp:347-350](../src/Engine/Options.cpp#L347). *(Persistence itself is fine —
  `tuReserved` is a raw int, so the value round-trips.)*
  **Decision (Jul 2026): not now** — a 5th reserve button does not fit the existing bottom bar, so
  this waits on the larger battle UI revamp rather than being wedged in.
  **Consequence to keep in mind:** the player cannot set a burst reserve at all, so **§4.1's reserve
  work is AI-only in practice**. It is still worth doing (the AI is the whole point of this
  feature), but do not expect to verify it from the player's reserve buttons — verify via the AI
  and a burst-only test weapon (§6.3).
- **No burst row in Ufopaedia** — [ArticleStateItem.cpp:263-270](../src/Ufopaedia/ArticleStateItem.cpp#L263)
  lists AUTO/SNAP/AIMED/MELEE only. The layout math at `:273` (`shift = (3 - current_row) * 16`)
  assumes ≤3 rows, so this is not a one-liner. StatsForNerds and InventoryState already handle burst.
- **No script binding** — [BattleItem.cpp:1667-1670](../src/Savegame/BattleItem.cpp#L1667) /
  [BattleUnit.cpp:7627-7629](../src/Savegame/BattleUnit.cpp#L7627) expose `battle_action_autoshoot`/
  `snapshot`/`aimshoot` but no `battle_action_burstshot` — **modders cannot reference burst from
  Y-Script.** Arguably the most valuable of the three.
- [SkillMenuState.cpp:129](../src/Battlescape/SkillMenuState.cpp#L129) — a skill with
  `targetMode: burst` shows no accuracy string.

---

## 5. Per-weapon AI targeting — closed as superseded

The other Phase 9 item proposed porting legacy's `aiRangeClose/Mid/Long/Max` +
`aiAttackPriorityClose/Mid/Long/Max` on `RuleItem`. **The audit closes it as superseded.** Recorded
here so the decision is not re-litigated.

First, a naming trap: despite the name, legacy's `aiAttackPriority*` lists are **fire-mode names**
(`"auto"`, `"aimed"`, `"snap"`, `"burst"`), *not* target-unit priorities. The legacy implementation
([legacy `AIModule::selectRangeOption`](file:///D:/Code/Projects/OpenXcomDX-Legacy/src/Battlescape/AIModule.cpp))
buckets distance into close(4)/mid(12)/long(20)/max, then walks the list and takes **the first mode
the unit can afford**. Defaults were `close: auto,aimed,snap` / `mid: snap,aimed,auto` /
`long+max: aimed,snap,auto`. It considers no accuracy, no shot count, and no TU efficiency.

DX already ships something strictly better. [`AIModule::extendedFireModeChoice`](../src/Battlescape/AIModule.cpp#L2590)
scores **every** affordable mode via [`scoreFiringMode`](../src/Battlescape/AIModule.cpp#L1698) as:

```
score = accuracy × numberOfShots × tuTotal / tuCost      // AIModule.cpp:1793
```

where `accuracy` is already range-banded — `calculateLimits()` supplies the per-mode
`aimRange`/`snapRange`/`autoRange`/`burstRange` window and `dropoff` decays accuracy outside it
([`:1714-1733`](../src/Battlescape/AIModule.cpp#L1714)) — then jittered by the unit's intelligence
and aggression ([`:2640`, `:2647`](../src/Battlescape/AIModule.cpp#L2640)). It covers DX's burst
mode, and it is already mod-gated by `ai: extendedFireModeChoice`.

In other words the engine derives the same range-band preference the legacy lists hand-authored,
from data the modder already supplies for the human player, and resolves ties on real expected
damage-per-TU instead of list order. Layering the legacy picker on top would let a cruder rule
pre-empt a better one.

The roadmap's *"(needs firing system)"* note was correct that Phase 5 was the blocker — but the
conclusion inverts: Phase 5 landing is what made this **obsolete**, not what unblocked it.

**Not done, deliberately.** If a mod ever needs to *force* a mode at a given range, the cheap
answer is a per-weapon bias added to `scoreFiringMode`'s return value, not a parallel selection
path. Left unbuilt until something asks for it.

### 5.1 What legacy never had — per-band *target* priority

Worth recording as a possible future feature, since the name misleads. There is genuinely **no**
target ranking in the engine today: [`selectNearestTarget`](../src/Battlescape/AIModule.cpp#L1380)
is a pure min-distance loop, and [`getTargetAttackWeight`](../src/Battlescape/AIModule.cpp#L2977)
— which already returns a weight and already has an `AiCalculateTargetWeight` script hook — is used
only as a **gate** (`validTarget`, [`:3029`](../src/Battlescape/AIModule.cpp#L3029)), never as a
ranking. Turning that gate into a ranking is a real, self-contained feature; it would be a
behavior change for every existing mod and would need its own flag and design doc. Out of scope
here.

---

## 6. Testing

No unit-test framework, so verification is in-play:

1. **Reserve correctness** — debug build, watch a hostile with a snapshot-less weapon (launcher).
   It must no longer strand a third of its TU, and must still fire.
2. **The reaction-fire fix** — the observable: aliens that advance and then react to a soldier
   moving into their view. Before the fix this is near-impossible in `AI_COMBAT`.
3. **Burst-only weapon** — the §4 sweep's single best test case. Author a test weapon with burst
   configured and no snap/auto, give it to a hostile, and confirm it: reports having ammo, reserves
   TU, reaction-fires, shoots while berserking, and makes the AI notice being shot. Each of those
   is a separate bug today, and one weapon exercises all of them.
4. **Player regression** — path-preview colors and the player's own reserve buttons must be
   byte-identical. This is the main risk surface (§3.1).
5. **Mod compatibility** — set `normalTUReserve: false` and confirm stock percentages return.

---

## 7. Docs to update on completion

- `DX-Features.md` — new entry for the reserve rework, with both option names and defaults.
- `DX-OXCE-Fixes.md` — **yes**. Two upstream deviations: the reserve rework itself (opt-in, so
  note the default), and spray targeting learning about burst (§4.2). The rest of the §4 burst gaps
  are DX-on-DX omissions rather than upstream deviations, so they belong in `DX-Features.md` under
  burst, not here.
- `bin/common/Language/DX/en-US.yml` — `STR_TIME_UNITS_RESERVED_FOR_BURST_SHOT` (§4.1), in the
  `#BattlescapeGame.cpp` section.
- `bin/standard/dx-test/dx-test.rul` — the `ai:` block turning both options on, with the usual
  commented "what to check" block, plus the burst-only test weapon from §6.3.
- `docs/Ruleset-AI.md` — new "TU reserve" section, keys tagged **[DX]**.
- `DX-Roadmap.md` — tick both Phase 9 items; note per-weapon targeting closed as superseded and
  counter-mind-control already shipped.
