# OpenXcom DX — Fixes to inherited OXCE code

This document records **bugs in the inherited OXCE / OXCE-Plus engine** that DX has either fixed or
deliberately chosen not to fix. It is deliberately narrow:

- It is **not** a feature list — DX's own features live in [DX-Features.md](DX-Features.md).
- It is **not** a changelog of DX code — only changes to *pre-existing upstream behavior* belong here.

Each entry states exactly what the code did, why that is wrong, what was changed (or why it wasn't),
and what a mod would observe differently. The point is that anyone diffing DX against upstream — or
merging upstream changes back in — can see precisely which behavioral deltas are intentional.

**Status legend:** ✅ fixed in DX · ⚠️ known, deliberately not fixed (none at present).

---

## ✅ `missionScripts:` / `arcScripts:` — `counterMin` / `counterMax` read uninitialized memory

**Where:** [`RuleMissionScript::RuleMissionScript`](src/Mod/RuleMissionScript.cpp) ·
[`RuleArcScript::RuleArcScript`](src/Mod/RuleArcScript.cpp)
**Consumed in:** [`GeoscapeState::determineAlienMissions`](src/Geoscape/GeoscapeState.cpp) (arc scripts
~L4169-4186, mission scripts ~L4381-4398)
**Found:** Jul 2026, while writing [docs/Ruleset-MissionScripts.md](docs/Ruleset-MissionScripts.md).

### What was wrong

Both classes declare

```cpp
int _counterMin, _counterMax;
```

with **no in-class initializer**, and neither constructor's initializer list mentions them. The
matching sibling class, `RuleEventScript`, *does* initialize them (`_counterMin(0), _counterMax(-1)`).
So for `missionScripts:` and `arcScripts:`, any script whose ruleset omits `counterMin:` /
`counterMax:` left those members holding **indeterminate values**.

The gating code then reads them:

```cpp
if (command->getCounterMin() > 0)                    // garbage may be > 0
{
    if (!missionVarName.empty() && counterMin > strategy.getMissionsRun(missionVarName))
        triggerHappy = false;
}
if (triggerHappy && command->getCounterMax() != -1)  // garbage is essentially never exactly -1
{
    if (!missionVarName.empty() && counterMax < strategy.getMissionsRun(missionVarName))
        triggerHappy = false;
}
```

### Why it mattered (and why nobody noticed)

The counters are only *reached* when the script also sets `missionVarName:` or `missionMarkerName:` —
otherwise the `!…empty()` guards short-circuit and the garbage is never compared. **No ruleset in
`bin/standard/` uses any of those four keys**, which is why the stock games are unaffected.

But the failure is very reachable for a mod that does use them, because the natural usage is to set
`missionVarName:` plus **one** bound — e.g. `counterMin: 3` ("only after 3 of these have run") and no
`counterMax:`. The unset bound is then garbage, `getCounterMax() != -1` passes, and the script's
eligibility is decided by whatever happened to be in memory: **non-deterministic, and the script can
silently never fire.** The mirror case (`counterMax:` set, `counterMin:` garbage) trips the `> 0`
guard the same way.

### What DX changed

Both constructors now initialize the pair to the same "no constraint" sentinels the checks already
test for, matching `RuleEventScript`:

```cpp
_counterMin(0), _counterMax(-1),
```

### Effect on mods

- A script that sets **both** counters: **no change whatsoever.**
- A script that sets **one or neither**, *and* uses `missionVarName:` / `missionMarkerName:`: the
  unset bound now reliably means "unbounded" instead of reading uninitialized memory. This is the
  behavior the field defaults already implied and that `eventScripts:` always had.
- Stock UFO/TFTD data: unaffected (the keys are unused there).

---

## ✅ `adhocScripts:` — only the first `adhocMissionScriptTags` entry was ever matched

**Where:** [`GeoscapeState::determineAlienMissions`](src/Geoscape/GeoscapeState.cpp#L4341-L4362)
**Found:** Jul 2026, while writing [docs/Ruleset-AdhocScripts.md](docs/Ruleset-AdhocScripts.md).

### What was wrong

When a geoscape event fires, the ad-hoc mission scripts it may trigger are filtered by a tag match:
the event carries a list of `adhocMissionScriptTags:`, each ad-hoc script carries its own list, and a
script should be eligible if *any* tag on the event matches *any* tag on the script. The loop:

```cpp
bool matchFound = false;
for (auto& atag : eventRules->getAdhocMissionScriptTags())
{
    for (auto& btag : command->getAdhocMissionScriptTags())
    {
        if (atag == btag) matchFound = true;
        break;                                // <-- unconditional
    }
    if (matchFound) break;                    // <-- correctly guarded
}
if (!matchFound) continue;
```

The inner `break` is **unconditional**, so the inner loop never runs a second iteration: each of the
event's tags is compared against the script's **`tag[0]` and nothing else**. Every later tag on an
ad-hoc script is structurally unreachable. This is not a "first match wins" *policy* — the comparisons
simply never happen. The asymmetry with the correctly-guarded outer loop (`if (matchFound) break;`) is
what gives it away as a typo rather than an intent.

The event side is fine: an event may list as many tags as it likes.

### What DX changed

The inner `break` is now guarded, so the loop does what the tag lists always implied — any event tag
may match any script tag:

```cpp
for (auto& btag : command->getAdhocMissionScriptTags())
{
    if (atag == btag)
    {
        matchFound = true;
        break;
    }
}
```

### Effect on mods

- An ad-hoc script with **one** tag: **no change whatsoever.** (Its only tag was already `tag[0]`.)
- An ad-hoc script with **several** tags: its 2nd and later tags now actually work, so it will fire
  for events it previously ignored. This **widens** matching.

That second case is the reason to be aware of this change when porting a mod from stock OXCE: a mod
that worked around the bug — by ordering tags so the effective one came first, or by leaving in tags
that appeared to do nothing — will behave differently under DX. Nothing in `bin/standard/` sets
`adhocMissionScriptTags:`, so the stock games are unaffected.

---

## ✅ Psi-amps trained **firing** experience

**Where:** [`TileEngine::awardExperience`](src/Battlescape/TileEngine.cpp) · reached from
[`TileEngine::hitUnit`](src/Battlescape/TileEngine.cpp)
**Found:** Jul 2026, while routing DX's mind blast through the standard hit path.

### What was wrong

`awardExperience` handles an explicit `experienceTrainingMode:` first. Failing that, it falls back to a
heuristic over the weapon's `battleType`, with cases for grenades/proxies (throwing), melee (melee) and
medikits (nothing) — and then a catch-all `else` labelled *"FIREARMS and other"* that picks a stat from
the weapon's `maxRange`:

```cpp
int maxRange = weapon->getRules()->getMaxRange();
if (maxRange > 10)
{
    expType = ETM_FIRING_100;
    expFuncA = &BattleUnit::addFiringExp;
}
```

There is **no `BT_PSIAMP` case**, so a psi-amp fell into that catch-all. Psi-amps are long-ranged, so
`maxRange > 10` held and the caster was awarded **firing** experience: a psi soldier got better with a
rifle by using a psi-amp on someone.

### Why it mattered (and why nobody noticed)

`awardExperience` is only reached for a psi-amp when the amp actually **damages** a unit, via `hitUnit`.
Panic and mind control never do — `ExplosionBState` zeroes their power — so the stock psi actions never hit
this path. It was reachable only through `BA_USE` on a damaging psi-amp, which no ruleset in
`bin/standard/` defines.

The upstream code is clearly *aware* of the gap without closing it:
[`TileEngine::psiAttack`](src/Battlescape/TileEngine.cpp) pointedly does **not** call `awardExperience`
in the default training mode, awarding psi skill directly instead, and calls it only when the modder set
an explicit mode. The heuristic simply cannot speak for psi.

### What changed

A `BT_PSIAMP` case was added to the heuristic, alongside the medikit one, returning **false** — no award:

```cpp
// PSI-AMPS
else if (weapon->getRules()->getBattleType() == BT_PSIAMP)
{
    return false;
}
```

In the default training mode the psi paths already award psi skill themselves (`psiAttack` for
panic/mind control, `mindBlast` for a blast), so returning false leaves that as the only award rather
than adding a bogus second one. An explicit `experienceTrainingMode:` is handled earlier and never
reaches this branch, so a mod that asks for a specific stat still gets exactly that.

### What a mod would observe

A damaging `BA_USE` psi-amp with **no** `experienceTrainingMode:` no longer trains firing. It still
trains psi skill through `psiAttack`, which was always the intended award. A mod that set an explicit
`experienceTrainingMode:` is unaffected. Nothing in `bin/standard/` defines such an amp, so the stock
games are unaffected.

---

## ✅ UFOpaedia articles crashed on an empty `image_id` instead of erroring

### What was wrong

`ArticleStateCraft`, `ArticleStateCraftWeapon` and `ArticleStateUnit` each blitted their background
without checking the lookup result:

```cpp
_game->getMod()->getSurface(defs->image_id)->blitNShade(_bg, 0, 0);
```

`Mod::getSurface` forwards to `Mod::getRule`, which **returns `0` for an empty name** rather than
throwing (the `error` parameter only governs the *not found* case). So an article of one of these
three types with no `image_id` at all dereferenced a null pointer and took the process down, with no
log line and no error dialog pointing at the offending article.

The sibling types don't share the bug: `ArticleStateVehicle` guards the empty case and falls back to
`BACK10.SCR`, `ArticleStateArmor` explicitly tests `image_id.empty()`, and the item/facility/UFO
articles hardcode their background and never read the field.

### Why it mattered (and why nobody noticed)

Every stock article of these types sets `image_id`, so the stock games never hit it. A mod that
omitted the key — easy to do, since it's optional on most other article types — got a hard crash on
opening the article rather than a message telling it what was missing.

### What DX changed

The three blits now null-check the surface they got back:

```cpp
if (Surface* bgImage = _game->getMod()->getSurface(defs->image_id))
{
    bgImage->blitNShade(_bg, 0, 0);
}
```

Deliberately **not** switched to `getSurface(name, false)`: that would also swallow the *not found*
case, turning a typo'd sprite name into a silently blank article. Keeping the default `error = true`
means a misspelled `image_id` still throws a clear "Sprite X not found", and only the genuinely
empty case is tolerated.

### Effect on mods

A previously-crashing article now opens with no background image drawn — everything else on the page
renders normally. Mods that set `image_id` correctly are unaffected. Typo'd sprite names still raise
the same exception they always did.

This also underpins **[DX] fallback pedia articles** (see [DX-Features.md](DX-Features.md)), which
can synthesize articles for rules with no authored entry.

---

## ✅ UFOpaedia prev/next recursed once per skipped article, unbounded

### What was wrong

`ArticleCommonState::nextArticle()` / `prevArticle()` stepped one index, then **called themselves
again** if the article they landed on was marked hidden:

```cpp
current_index++;            // (or wrap to 0)
if (isCurrentArticleHidden())
{
    nextArticle();
}
```

Two problems. Recursion depth grows with the number of *consecutive* hidden articles, so a long run
of them meant a stack frame each. And if **every** article was hidden there was no base case at all
— infinite recursion into a stack overflow.

### Why it mattered (and why nobody noticed)

Upstream, the only way to mark an article hidden is the player toggling entries one at a time in
the pedia list, so a run long enough to matter — let alone hiding literally everything — was not
realistically reachable. The `Ufopaedia::next`/`prev` wrappers also bail out when the *current*
article is hidden, which masks the all-hidden case in practice.

DX makes both cases easy to hit: unlisted generated articles are marked with the same flag, and
they cluster at the end of the index (they are assigned list order last), so stepping past them
recursed once per entry — potentially thousands on a large mod.

### What DX changed

Both functions became bounded loops, capped at one pass over the list:

```cpp
for (size_t steps = articleList.size(); steps > 0; --steps)
{
    // ... step one index, wrapping ...
    if (!isCurrentArticleHidden())
    {
        return;
    }
}
```

Same traversal and same landing spot, but it can neither overflow the stack nor spin forever; if
every article is hidden it simply stops after a full pass.

### Effect on mods

None visible. Navigation lands on exactly the same article it did before in every case that
previously terminated; the cases that changed are the ones that used to crash.

---

## ✅ AI TU reserve — flat percentage of max TU, and never reserved outside patrol

**Where:** [`BattlescapeGame::checkReservedTU`](src/Battlescape/BattlescapeGame.cpp) ·
[`AIModule::think`](src/Battlescape/AIModule.cpp)
**Found:** Jul 2026, auditing Phase 9 of [DX-Roadmap.md](DX-Roadmap.md).
**Opt-in:** `ai: normalTUReserve` / `ai: combatTUReserve`, both default `false` (see
[docs/Ruleset-AI.md](docs/Ruleset-AI.md#tu-reserve)).

### What was wrong

Two independent defects that compound into one very visible symptom.

**1. The reserve is a percentage of the wrong number.** `checkReservedTU` has a hostile-only
early-return that reserves a flat fraction of `getBaseStats()->tu` — the unit's *maximum* TU:

```cpp
case BA_SNAPSHOT:  cost.Time += (bu->getBaseStats()->tu / 3);      break; // 33%
case BA_AUTOSHOT:  cost.Time += ((bu->getBaseStats()->tu / 5)*2);  break; // 40%
case BA_AIMEDSHOT: cost.Time += (bu->getBaseStats()->tu / 2);      break; // 50%
```

It never consults the weapon, so a cheap pistol reserves as much as a heavy cannon, and a wounded
or encumbered unit reserves the same absolute amount as a fresh one. It also `return`s before the
player's fallback chain (autoshot → snapshot → aimed → kneel → none), so a weapon with a
**non-standard shot config** — a launcher or a burst-only weapon with no snapshot — still reserves
33% of max TU for a shot it can never fire. The preceding `cost.updateTU()` is discarded outright
by `cost.Time = tu`.

**2. The AI only reserves while patrolling.** `AIModule::think` resets `_reserve = BA_NONE` and
then sets it in exactly one branch, `AI_PATROL`. `AI_COMBAT`, `AI_AMBUSH` and `AI_ESCAPE` all leave
it `BA_NONE`.

### Why it mattered (and why nobody noticed)

Defect 2 is the cause of the long-standing "aliens never reaction fire" complaint, and the
connection is not obvious because **the reaction code itself is correct**.
`TileEngine::determineReactionType` checks the real shot cost via `BattleActionCost(...).haveTU()`
and applies no reserve logic at all — which is right. But a unit in `AI_COMBAT` reserves nothing,
so `UnitWalkBState`'s per-step gate lets it walk to ~0 TU; by the time a soldier moves into its
view, `haveTU()` fails and it silently never reacts. The bug is upstream of the reaction path, so
inspecting the reaction path finds nothing wrong.

Defect 1 hides because vanilla weapons all have a snapshot, so the fallback chain being skipped is
invisible until a mod ships a weapon without one.

### What DX changed

Both fixes are **opt-in and default to off**, so stock behavior is preserved on upgrade.

With `normalTUReserve`, the hostile branch keeps only the `getReserveMode()` lookup and falls
through to the same actual-shot-cost path the player uses. With `combatTUReserve`, `_reserve` is
also set in `AI_COMBAT` and `AI_AMBUSH`. In combat it is applied **only when the AI is
repositioning** (pending action is a move) — reserving on top of a shot the unit is already about
to take would deadlock it.

The aggression → shot-mode mapping moved into a new `AIModule::pickReserveMode()`, which also
**degrades the chosen mode to one the weapon actually has** (Auto → Burst → Snap → Aimed, matching
`RuleItem::getDualFireMode`). Reserving for an unconfigured mode costs 0 TU, i.e. reserves nothing —
which is how a burst-only weapon previously ended up with no reserve at all. Melee weapons now
reserve their hit, so a charging unit doesn't arrive with no TU to swing.

### Effect on mods

None unless opted in. With the options on, aliens hold back enough TU to shoot after moving and
reaction-fire far more often — a real difficulty increase, so it is worth rebalancing against.
`BA_BURSTSHOT` also gained a case in the retained stock percentage switch (37%, between snap's 33%
and auto's 40%), so a mod that leaves `normalTUReserve` off but uses burst weapons no longer
reserves 0.

---

## ✅ Spray targeting was gated on auto shots only

**Where:** [`BattlescapeGame::handleAction` / waypoint handling](src/Battlescape/BattlescapeGame.cpp)
**Found:** Jul 2026, during the Phase 9 burst-mode gap sweep.

### What was wrong

`sprayWaypoints` lets a multi-round volley be walked along a line of waypoints. Both the gate that
*starts* spray targeting and the one that *cancels* a waypoint tested `_currentAction.type ==
BA_AUTOSHOT` exclusively, so a **burst** could never use spray targeting.

### What DX changed

Both gates now accept `BA_AUTOSHOT` or `BA_BURSTSHOT`. Everything downstream already keys on the
`sprayTargeting` flag rather than the action type, so no other change was needed.

This is upstream code that simply predates DX's burst mode — the omission is an oversight, not a
design decision, since burst is a multi-round volley in exactly the way spray targeting exists to
serve.

### Effect on mods

A weapon with `sprayWaypoints` set can now spray with its burst mode as well as its auto mode.
Nothing that worked before changes.

---

## ✅ Camera moved to things the player could not see (position leaks)

**Where:** [`ExplosionBState::init`](src/Battlescape/ExplosionBState.cpp) ·
[`BattlescapeGame::checkForPanic`](src/Battlescape/BattlescapeGame.cpp) ·
[`UnitWalkBState::think`](src/Battlescape/UnitWalkBState.cpp)
**Found:** Jul 2026, auditing camera behavior for [plans/Feature-BattleCamera.md](plans/Feature-BattleCamera.md).
**Gated by:** `battleCameraDirection` (default on); with it off, the stock behavior is preserved.

### What was wrong

The tactical camera's automatic moves guarded visibility inconsistently — three different policies,
two of them wrong, all able to reveal an unspotted position by panning to it:

- **Explosions** centered unconditionally. A big explosion (or a chained one) in unexplored territory
  pulled the camera to it, even though `_explosionInFOV` was computed and available.
- **Panic** centered on a panicking/berserking unit whenever the infobox showed — and with the
  default `noAlienPanicMessages` off, the infobox shows for **invisible** aliens, so the camera panned
  to an unseen alien's exact tile. The guard was effectively backwards.
- **View level** followed *every* unit's floor change in `UnitWalkBState`, with no visibility check
  (only the horizontal follow beside it was guarded). An unseen alien taking a lift or stairs yanked
  the player's view level to that floor.

Deaths, by contrast, guarded correctly (`if (!getVisible() && !debug) return;`) — that was the model
the others should have followed.

### What DX changed

All automatic camera moves now route through one helper, `Map::focusCamera`, which enforces a single
visibility rule: the player's own turn shows everything, otherwise the target tile/unit must be
spotted (mirroring the existing `_projectileInFOV` rule), debug excepted. Explosions, impacts, panic
and the actor beat all use it; the view-level follow gained the same `own-unit || visible || debug`
guard.

### Effect on mods / players

With `battleCameraDirection` on (default), the camera no longer reveals unspotted positions via
explosions, panic messages, or floor changes. With it off, the stock (leaky) behavior is unchanged.

---

## ✅ Minor camera warts: HUD-occluded on-screen test, and a stale smooth-camera cache

**Where:** [`Camera::isOnScreen`](src/Battlescape/Camera.cpp) · [`Map`](src/Battlescape/Map.cpp)
**Found:** Jul 2026, same audit.

### What was wrong

1. `Camera::isOnScreen(pos, /*unitWalking=*/false, ...)` — the branch used by "should I re-center?"
   callers — tested against `_screenHeight`, so a tile hidden **behind the HUD icon panel** counted as
   on screen. Camera-framing decisions based on it (e.g. the dying-unit focus) could therefore decide a
   subject was visible when it was actually occluded, and skip a move that should have happened. The
   sibling `unitWalking=true` branch already handled the icon panel (and its side gutters) correctly.
2. `Map::_smoothCamera` cached `Options::battleSmoothCamera` **once at construction**, so toggling the
   option mid-battle had no effect until the next mission.

### What DX changed

1. Camera-framing callers now use the HUD-aware `unitWalking=true` branch; the broken `false` branch is
   simply no longer used for framing (it is left as-is for any non-camera callers).
2. The cache was removed; `Options::battleSmoothCamera` is read live at the point of use.

### Effect on mods / players

Off-screen framing decisions now account for the HUD, and the smooth-camera toggle takes effect
immediately. No ruleset surface involved.

---

*If you fix an upstream bug, add an entry here in the same shape — what the code did, why it is
wrong, what changed, and what a mod would notice — and cross-link it from the relevant
[ruleset doc](docs/Ruleset.md).*
