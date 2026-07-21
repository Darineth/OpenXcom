# Feature: Battlescape camera direction

**Status:** ✅ **Implemented (Jul 2026)** — builds clean; pending in-game validation (§5).
Option `battleCameraDirection` (Battlescape, default **on**).

### Implementation notes (deltas from the design above)

- **Claim tracking stayed derived, not stored.** `Map::getCameraClaim()` computes the current claim
  from the two states that already have lifetimes (`_deathFocus`, non-empty `_explosions`); ACTOR/
  IMPACT/PROJECTILE are momentary `focusCamera()` calls, so no claim/release bookkeeping was needed.
  The projectile gate became `getCameraClaim() <= CameraClaim::PROJECTILE`, a faithful rewrite of the
  old `_explosions.empty() && !_deathFocus` triple.
- **No new `Camera` method.** The HUD wart (Q5) was fixed simply by having camera-framing callers use
  the existing `isOnScreen(pos, /*unitWalking=*/true, ...)` branch, which already accounts for the
  icon panel and its side gutters — the `unitWalking=false` branch (which tests `_screenHeight`) was
  the only broken one, and nothing camera-facing needs it. `Camera.cpp` was left untouched.
- **The `ACTOR` beat lives in `ProjectileFlyBState::init` only** — i.e. projectile attacks. Melee and
  psi were deliberately left out: a melee attacker is adjacent to its target, so the `IMPACT` beat
  frames both; and for psi the meaningful beat is *who got hit* (the `IMPACT` on `_targetPsiOrHit`),
  not the caster. This keeps the change small and matches where the gap actually is.
- **Mind-control / panic success is covered by the generalised `IMPACT` beat**, not a new hook: MC and
  panic land through a `BT_PSIAMP` hit whose `ExplosionBState` sets `_targetPsiOrHit`, so framing the
  hit unit already frames the controlled/broken target. Morale-triggered panic (no psi) is the
  separate `BattlescapeGame` path, fixed there.
- **Proximity grenades are covered by the `EXPLOSION` claim, not a distinct `ACTOR` beat.** A tripped
  prox `statePushNext`es an `ExplosionBState` with a zero fuse, so the blast frames immediately (and
  is visibility-gated against leaks). A separate pre-blast frame of the same tile one draw earlier
  would show nothing extra, so it was dropped — the Q3 intent ("explain a surprise explosion") is met.
- **Stun/collapse came for free**: `UnitDieBState` already plays the unconscious collapse, so routing
  its focus through the `DEATH` claim frames knockouts as well as kills.
- **Reaction shots no longer snap the camera back** (added after first playtest). A shot restores the
  pre-shot camera offset when it finishes (`ProjectileFlyBState` / `MeleeAttackBState`, via
  `BattleAction::cameraPosition`). For a *deliberate* player shot that is correct — return to your
  unit after watching the shot. But for a **reaction** the saved offset is wherever the interrupted
  action had dragged the camera (e.g. where an alien's earlier missed burst vanished), so restoring it
  snaps to a meaningless spot the instant the reaction's own shots clear. Reactions (`_action.reaction`)
  now skip the restore and leave the camera where their beats ended; the next event (continued movement,
  another shot) drives it from there. Only reactions are affected — alien AI actions never set
  `cameraPosition` (they keep the `(0,0,-1)` sentinel and never restored anyway), and deliberate player
  shots still return to the shooter. Gated by `battleCameraDirection`.
**Roadmap:** [DX-Roadmap.md](../DX-Roadmap.md) — New Features.
**Supersedes:** [dying-unit-camera-focus.md](dying-unit-camera-focus.md) (shipped; its behavior is folded
into the ladder below as the `DEATH` claim).

Goal: the tactical camera should **show the player what matters, and otherwise hold still**.

---

## 1. Motivation

The camera has accreted one ad-hoc rule per feature over years — `git log` reads "camera thing",
"more camera tweaks", "fix camera 'onscreen' determination", "center camera", "Reset the camera too"
(twice). There is no shared notion of *what the camera is currently trying to show*, so each new
feature adds another `centerOnPosition` call and another boolean to suppress somebody else's.

The concrete gap that prompted this: **a reaction shot never frames the reacting unit.** An alien
reaction-fires from off screen and the player sees nothing — not the shooter, not the muzzle, only
(maybe) a bullet crossing a visible tile, and then the camera snaps back to where it was. The most
dramatic thing in the game happens off camera.

Five goals, which genuinely conflict:

| # | Goal | Conflicts with |
|---|---|---|
| 1 | Show units taking actions | 2 (shooter and target can't both be framed at range) |
| 2 | Show units hit by actions | 1, 4 (a burst hitting three units shouldn't pan three times) |
| 3 | Track bullets in flight | 4 (bullet chasing is the largest single source of camera motion) |
| 4 | Don't move unnecessarily | 1, 2, 3 |
| 5 | Show other key events | 4 |

Goal 4 is not a tiebreaker to apply after the others — it is the **default state**. The design below
makes "hold still" the base case and requires every move to justify itself.

---

## 2. Audit — what ships today (Jul 2026)

Confirmed by reading the engine, not `Extended.txt`.

### 2.1 `Camera` is a dumb primitive

[`Camera::centerOnPosition`](../src/Battlescape/Camera.cpp#L420) is an **unconditional hard snap** —
no easing, no "already visible" short-circuit, no distance threshold. It also sets
`_mapOffset.z = _center.z`, so **every centre is a 3-D move that forces the view level**, conflating
two concerns. `jumpXY` skips the map-bounds clamp that `scrollXY` applies, so event-driven motion can
park the viewport off the map edge while user scrolling cannot.

[`Camera::isOnScreen`](../src/Battlescape/Camera.cpp#L572) is the existing "is this framed?" test and
has two behaviors. The `unitWalking = true` branch hand-corrects for the HUD icon panel. **The
`unitWalking = false` branch — the one every "should I re-centre?" caller uses — tests against
`_screenHeight`, not `_visibleMapHeight`** ([`:614-618`](../src/Battlescape/Camera.cpp#L614)), so a
tile **hidden behind the HUD counts as on screen** and suppresses a re-centre that should happen.
*(verified)*

There is no `_traceSlider`; smooth-camera state lives on `Map` (`_followProjectile`, `_launch`,
`_smoothCamera`, `_smoothingEngaged`, `_deathFocus`).

### 2.2 The projectile chase, and the only priority system that exists

All bullet following is in [`Map::drawTerrain`](../src/Battlescape/Map.cpp#L1158), gated by:

```cpp
if (_explosions.empty() && !_deathFocus && _projectileInFOV && _followProjectile)
```

That line **is** the current priority system: three booleans, each added by a different feature to
outrank the one before (explosions beat bullets; deaths beat both). It works, but it does not
generalise — every new claimant needs another flag and every existing claimant needs to learn about it.

`_smoothCamera` is cached from the option at construction
([`Map.cpp:170`](../src/Battlescape/Map.cpp#L170)), so **toggling `battleSmoothCamera` mid-battle does
nothing**.

### 2.3 Where the camera moves today

~25 call sites. The load-bearing ones:

| Site | Behavior | Guards? |
|---|---|---|
| [`Map::drawTerrain:1158-1224`](../src/Battlescape/Map.cpp#L1158) | Bullet chase (smooth lock-to-centre, or OG page-flip) | `_projectileInFOV` |
| [`UnitDieBState::focusCamera`](../src/Battlescape/UnitDieBState.cpp#L151) | Frame an off-screen death, claim from projectile | **visibility + on-screen** ✅ |
| [`ExplosionBState:298`](../src/Battlescape/ExplosionBState.cpp#L298) | Centre on big explosion | **none** — pans to explosions in unexplored territory |
| [`ExplosionBState:405`](../src/Battlescape/ExplosionBState.cpp#L405) | Centre when a hostile hits a player unit | none |
| [`UnitWalkBState:192`](../src/Battlescape/UnitWalkBState.cpp#L192) | Follow **non-player** units only | visibility + on-screen |
| [`UnitWalkBState:195`](../src/Battlescape/UnitWalkBState.cpp#L195) | `setViewLevel` every step | **none** — an unseen alien changing Z yanks the player's view level |
| [`BattlescapeGame:1739`](../src/Battlescape/BattlescapeGame.cpp#L1739) | Centre on panicking unit | `getVisible() \|\| !noAlienPanicMessages` — **backwards**: leaks invisible aliens |
| `BattlescapeGame:356/495/720/1459` | Unit selection / turn transitions | n/a |
| `BattlescapeState` (many) | Manual: drag, edge scroll, level buttons, next/prev unit (Shift suppresses), centre-on-enemy buttons | n/a |

`AIModule` contains **zero** camera references — all alien-turn motion is emergent from the above.

### 2.4 Reaction fire frames nothing — *the headline gap*

[`TileEngine::tryReaction:3127`](../src/Battlescape/TileEngine.cpp#L3127) does the only camera-adjacent
thing in the whole file:

```cpp
action.cameraPosition = _save->getBattleState()->getMap()->getCamera()->getMapOffset();
```

That is a **save-for-restore, not a move**. The queued `ProjectileFlyBState` / `MeleeAttackBState`
never centres. So an off-screen reactor shooting at an off-screen tile produces **zero camera motion**,
and then the restore snaps back. DX **overwatch** goes through the identical path and is equally blind.

Related: `cameraPosition` defaults to the sentinel `(0,0,-1)`, but is always assigned from
`getMapOffset()`, whose `.z` is a view level in `[0, mapsize_z)` and therefore **never −1**. So the
`z != -1` guard in every consumer always passes and **the restore always fires**. *(verified)* The only
place the sentinel is restored is the cancel path
([`BattlescapeGame.cpp:1157`](../src/Battlescape/BattlescapeGame.cpp#L1157)).

### 2.5 Visibility gating is inconsistent

Three different policies for the same question:

- **Deaths** guard correctly: `if (!getVisible() && !getDebugMode()) return;` with the comment *"don't
  pan to deaths the player can't see - that would give away unspotted positions."* This is the model.
- **Explosions** don't guard at all (`_explosionInFOV` is computed but `ExplosionBState:300` never reads it).
- **Panic** guards backwards — with the default `noAlienPanicMessages` it centres on *invisible* aliens.
- **`setViewLevel` in `UnitWalkBState`** has no guard.

Note the map-draw gate ([`Map.cpp:707`](../src/Battlescape/Map.cpp#L707)) shows the "Hidden Movement"
panel *instead of* the map — but **the camera still moves underneath**, it is merely not rendered. So a
leak is invisible at the time and becomes apparent the moment the panel clears.

### 2.6 Verdict

Nothing upstream to reuse: OXCE has no camera-direction concept, no `battleAutoCenter`, no per-event
centring toggles. The options that exist (`battleSmoothCamera` default **off**, `battleEdgeScroll`,
drag-scroll settings) tune *mechanism*, not *policy*. The `followProjectiles:` per-fire-mode ruleset key
and the Alt-key override are the only existing policy knobs. This is a true DX delta.

---

## 3. Design — a claim ladder

### 3.1 The three invariants

Every automatic camera move must pass all three. These already exist informally in the dying-unit
feature; the change is making them **universal and enforced in one place**.

- **V (Visible).** Never move to show something the player cannot see. Never reveal an unspotted
  position — including via view-level changes, which are currently unguarded.
- **F (Framed).** Never move if the subject is **already on screen**. This is the whole of goal 4:
  an in-view firefight gets no camera motion at all.
- **C (Claim).** At most one thing owns the camera at a time. A higher claim takes it; a lower claim
  cannot move it until the higher one releases.

### 3.2 One action is a *sequence* of beats, not a contest

The key insight from review (**Q1**): for a single attack, the beats are **sequential**, not
competing. An attack plays out as

> **ACTOR** (who is shooting) → **PROJECTILE** (the shot travelling) → **IMPACT** (where it landed)
> → **DEATH** (if it killed)

Each beat hands off to the next as the action progresses. There is no shooter-vs-target conflict to
resolve, because they happen at different *times*. A reaction shot is therefore **just a normal shot
that begins with an actor beat the player didn't ask for** — frame the reactor, follow the bullets,
land on the target, exactly like any other shot.

This matters for the player's mental model: the camera behaves identically for every attack, so
there is one rule to learn rather than a special case for reactions.

Note that for the player's *own* shots the `ACTOR` beat is a **no-op**: the player just gave the
order, so the shooter is already framed and invariant **F** suppresses the move. The beat only
becomes visible for shots the player didn't initiate — which is precisely the gap being fixed.

### 3.3 The ladder (for *simultaneous* claims)

Priority only arbitrates when two **different** actions overlap — DX's async projectiles, dual fire,
staggered bursts and chained explosions all make this possible. Highest wins.

| Priority | Claim | Raised by | Released |
|---|---|---|---|
| 5 | `DEATH` | `UnitDieBState::init` | `deinit` |
| 4 | `EXPLOSION` | `ExplosionBState::init` | state pop |
| 3 | `IMPACT` | A unit is hit (`ExplosionBState` hit path) | state pop |
| 2 | `ACTOR` | Attack begins — **including reaction fire and overwatch** | first projectile spawns, or state pop |
| 1 | `PROJECTILE` | Bullet in flight (`Map::drawTerrain`) | no projectiles remain |
| 0 | `IDLE` | — | — |

Rationale: later beats are more informative than earlier ones (a death tells you more than a muzzle
flash), so when two actions collide the more advanced one keeps the camera. This preserves today's
behavior exactly — the existing `_explosions.empty() && !_deathFocus && _followProjectile` triple is
a subset of this ordering — making the ladder a refactor at the bottom and an extension at the top.

**No `MANUAL` tier.** Considered and rejected. The player *can* scroll during the alien turn —
`Map::mouseOver`/`mousePress`/`keyboardPress` pass straight to the camera with no turn gating
([Map.cpp:2836](../src/Battlescape/Map.cpp#L2836)); only the HUD buttons are gated by
`allowButtons`'s `getSide() == FACTION_PLAYER` check. But during the alien turn showing the player
what happened *is* the point, and during their own turn every camera move follows from an order they
just gave. A lockout would suppress exactly the beats this feature exists to deliver.

### 3.4 Mapping the five goals

**Goal 1 — show units taking actions.** The `ACTOR` beat. When an attack begins, if the actor is
visible and off screen, centre on them. Fixes reaction fire and overwatch, and gives alien-turn shots
a "who is shooting" beat.

**Goal 2 — show units hit.** The `IMPACT` beat, reached naturally at the end of the sequence.
To protect goal 4, `IMPACT` **re-centres at most once per action** — the first impact claims;
subsequent hits from the same burst do not re-pan.

**Goal 3 — track bullets.** Unchanged mechanically; formalised as the lowest claim, which it already
effectively is. Keeps `followProjectiles:` and the Alt override.

**Goal 4 — don't move unnecessarily.** Invariant **F** (already framed → never move) plus the
once-per-action impact rule. Note **F** makes goal 4 self-enforcing rather than a judgement call.

**Goal 5 — other key events.** Decided (Q3): **mind-control/panic success**, **stun/collapse**, and
**proximity-grenade trigger**. See §3.6.

### 3.5 Per-weapon opt-out (Q2)

The `ACTOR` beat reuses the existing per-fire-mode **`followProjectiles:`** ruleset key rather than
adding a new one. A weapon that already declares itself too twitchy to follow ("e.g. minigun-like
weapons", per the comment at
[ProjectileFlyBState.cpp:427](../src/Battlescape/ProjectileFlyBState.cpp#L427)) also skips the actor
beat, which is the same intent. No new ruleset surface, so no `docs/Ruleset-*.md` change. The Alt-key
override suppresses both for the same reason.

### 3.6 Additional key events (Q3)

Three events join the ladder; each reuses an existing claim tier rather than adding one.

| Event | Claim | Notes |
|---|---|---|
| **Mind control / panic succeeds** on a visible unit | `IMPACT` | A unit switching sides or breaking is a major swing that is currently near-invisible. Panic already centres at [BattlescapeGame.cpp:1739](../src/Battlescape/BattlescapeGame.cpp#L1739) but with a **backwards** guard that leaks invisible aliens — routing it through `focusCamera` fixes that as a side effect. |
| **Stun / collapse** (not death) | `DEATH` | Same beat as a death and currently unframed. Reuses `UnitDieBState`'s existing path, which already handles the unconscious case, so this is close to free. |
| **Proximity grenade triggers** | `ACTOR` | Frames the tripped grenade *before* it detonates, explaining a surprise explosion. `EXPLOSION` then takes over for the blast — the same actor → impact handoff as a shot. |

**Newly-spotted hostiles were considered and rejected**: on a busy map first-contact fires constantly
and would dominate the camera, working against goal 4.

### 3.7 Implementation shape

Deliberately **small** — a claim holder, not a framework. DX's standing preference is targeted deltas
over generic subsystems, and the temptation here is to build a whole director.

New on `Map` (where the existing camera-policy flags already live), replacing `_deathFocus`:

```cpp
enum class CameraClaim { IDLE = 0, PROJECTILE, ACTOR, IMPACT, EXPLOSION, DEATH };

// returns true if the claim was taken (caller may then move the camera)
bool Map::claimCamera(CameraClaim who);
void Map::releaseCamera(CameraClaim who);   // no-op if `who` doesn't hold it
CameraClaim Map::getCameraClaim() const;
```

and one shared helper that enforces **V** and **F** so no call site has to remember them:

```cpp
// centre only if the player can see `pos`, it is not already framed, and `who` can take the claim
bool Map::focusCamera(CameraClaim who, Position pos, const BattleUnit *subject = nullptr);
```

Then:
- `Map::drawTerrain:1158` becomes `getCameraClaim() <= CameraClaim::PROJECTILE && _projectileInFOV && _followProjectile`
  — replacing `_explosions.empty() && !_deathFocus`.
- `UnitDieBState::focusCamera` becomes a `focusCamera(DEATH, ...)` call (same behavior, less code).
- `ExplosionBState`, the new actor hook, and the impact hook all route through `focusCamera`, which
  fixes the missing visibility guards in passing.

**Fixes folded in** (each small, each currently a latent bug):
1. **The `isOnScreen` HUD wart** — the `unitWalking = false` branch tests `_screenHeight`, so tiles
   behind the HUD count as framed and wrongly suppress a re-centre. **Decision: add a camera-only
   variant** (or a parameter) testing `_visibleMapHeight`, and leave the walking path untouched —
   `isOnScreen` is shared with sound/animation gating and sprite-cache boundaries in
   `UnitWalkBState`, where a global change is easy to regress and hard to notice.
2. Separate view-level changes from `centerOnPosition`, or at least guard the unconditional
   `setViewLevel` in `UnitWalkBState:195` on visibility.
3. Fix the panic guard's inverted logic (falls out of routing panic through `focusCamera`, §3.6).
4. Read `Options::battleSmoothCamera` live instead of caching it at construction.
5. Either honour the `cameraPosition` sentinel or drop it — today it is dead weight that always restores.

### 3.8 Gating

One new option, `battleCameraDirection` (Battlescape, **default on**), covering the new `ACTOR` and
`IMPACT` claims and the §3.6 events. The existing `battleFocusDyingUnits` stays as-is so the shipped
feature keeps its switch. Off = today's behavior.

---

## 4. What this does *not* do

- **No easing/smooth panning.** `centerOnPosition` stays a hard snap. Interpolated camera movement is a
  much larger change and orthogonal to *deciding where to look*.
- **No fog-of-war camera suppression.** The legacy fork suppressed forced camera moves under fog;
  [Feature-FogOfWar.md](Feature-FogOfWar.md) explicitly puts that out of scope, and invariant **V**
  covers the important half.
- **No rework of manual scrolling**, drag, or edge-scroll mechanics.

---

## 5. Testing

In-play, no test framework:

1. **Reaction fire** — the headline case. An alien reaction-fires from off screen at a soldier: the
   camera should frame the alien, then the impact. Compare with the option off.
2. **Overwatch** — same, via the DX overwatch path.
3. **No jerk when framed** — a firefight entirely inside the viewport should produce **zero**
   automatic camera motion.
4. **Multi-hit burst** — an auto/burst volley hitting several units pans at most once.
5. **No leaks** — an unseen alien shooting, dying, panicking, or changing level must not move the
   camera *or the view level*. Check by watching where the camera sits when Hidden Movement clears.
6. **Regression** — deaths still behave exactly as the shipped feature does.

---

## 6. Docs to update on completion

- `DX-Features.md` — new camera-direction entry; fold in the existing dying-unit entry.
- `DX-OXCE-Fixes.md` — the inconsistent visibility guards, the `isOnScreen`/HUD wart, and the
  cached `battleSmoothCamera` are all pre-existing upstream defects.
- `bin/common/Language/DX/en-US.yml` — `STR_CAMERA_DIRECTION` + tooltip.
- `DX-Roadmap.md` — tick; mark `dying-unit-camera-focus.md` superseded.
- No ruleset keys planned, so no `docs/Ruleset-*.md` change (unless Q2 adds a per-weapon key).

---

## 7. Decisions (Jul 2026)

All resolved in review; recorded so they are not re-litigated.

**Q1. On a reaction shot, shooter or target? → *Both, sequentially.*** A reaction shot frames the
shooter, follows the bullets, and lands on the target — identical to any other shot. This reframed
the design: the beats of one action are a **sequence**, not a priority contest (§3.2), and the ladder
only arbitrates between *overlapping* actions. One camera rule for every attack.

**Q2. Per-weapon opt-out? → *Reuse `followProjectiles:`.*** No new ruleset key (§3.5).

**Q3. Which other events? → *Mind-control/panic success, stun/collapse, proximity-grenade trigger*** (§3.6).
**Newly-spotted hostiles rejected** — first contact fires too often on a busy map and would fight goal 4.
Fire/terrain spread and a separate overwatch-trigger beat also rejected (noisy; redundant with `ACTOR`).

**Q4. `MANUAL` tier? → *No, dropped.*** Verified that the player *can* scroll during the alien turn
(`Map`'s input handlers have no turn gating; only HUD buttons are gated). But automatic framing should
win then — that is the point of the alien turn — and during the player's own turn every move follows
an order they just gave. A lockout would suppress the beats this feature exists to deliver. (§3.3)

**Q5. `isOnScreen` HUD wart? → *Camera-only variant.*** Leave the walking path alone (§3.7 fix 1).

### Remaining risk

The `ACTOR` beat's frequency is the thing most likely to need tuning in play. Every off-screen alien
attack now costs one camera move. Invariant **F** means an in-view firefight is unaffected, and
`followProjectiles: false` gives mods an escape hatch, but if a heavy alien turn reads as too busy the
first lever is to restrict `ACTOR` to attacks that actually target a player unit.
