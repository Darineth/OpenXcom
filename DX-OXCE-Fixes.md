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

*If you fix an upstream bug, add an entry here in the same shape — what the code did, why it is
wrong, what changed, and what a mod would notice — and cross-link it from the relevant
[ruleset doc](docs/Ruleset.md).*
