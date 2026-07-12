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

*If you fix an upstream bug, add an entry here in the same shape — what the code did, why it is
wrong, what changed, and what a mod would notice — and cross-link it from the relevant
[ruleset doc](docs/Ruleset.md).*
