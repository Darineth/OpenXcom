# OpenXcomDX — New Air-Combat Minigame Design

*NOTE*: This design document is based on the original full implementation.  It does 
not mean this is how it should be implemented if we try again!  It may reference
files and lines of code that do not exist in the modern DX codebase.

This is **just a starting point**.

> **Scope:** A mechanics-level design description of the turn-based air-combat
> minigame that replaces stock OpenXcom's real-time interception dogfight. This is
> the deep companion to §22 of [../Legacy-DX-Features.md](Legacy-DX-Features.md);
> that section is the short index entry, this is the full design.
>
> **Focus:** *what the game does and how it plays*, not a line-by-line code tour.
> Concrete numbers below were read out of the source (Jun 2026) and are cited so
> they can be re-checked, but the emphasis is mechanical. Where a system is
> half-wired or stubbed in the code, it is called out under
> [Unfinished / rough edges](#unfinished--rough-edges).
>
> **Status:** Implemented but **off by default.** See
> [Status & how to turn it on](#status--how-to-turn-it-on).

---

## 1. Concept

Stock OpenXcom resolves interceptions in real time: a craft closes on a UFO along a
distance slider while you pick standoff / cautious / standard / aggressive stances
and the auto-firing weapons trade hits until someone breaks off.

OpenXcomDX replaces that with a **turn-based pursuit duel** fought on a one-
dimensional **range axis**. Each combatant — your interceptor(s), the target UFO,
and any escort UFOs — takes discrete turns spending **Time** and **fuel** to
maneuver along the axis and fire weapons. It plays like a small tactical
mini-battlescape for the skies: positioning relative to weapon ranges is the core
decision, and either side can try to run.

The fantasy: instead of a slider that mostly resolves itself, you *fly* the
engagement — close the distance to bring a short-range cannon to bear, or back off
to snipe with long-range missiles while the UFO burns the fuel it needs to catch
you.

---

## 2. The combat space

### The range axis

Combat happens on a horizontal line of positions. The player side sits on the left,
the enemy side on the right; **range between two units is simply the absolute
difference of their positions** ([`executeAction`, AirCombatState.cpp:1420](src/Geoscape/AirCombatState.cpp#L1420)).

```
 PLAYER SIDE                                        ENEMY SIDE
   0      1      2      3      4      5      6      7
  [you]  ──►  closing the gap  ──►            [escort][UFO]
 standoff                                      ▲ escort  ▲ primary UFO
```

- **Position 0 is the "standoff" row** on the player's side. Interceptors enter
  combat here ([`addCraft`, AirCombatState.cpp:866](src/Geoscape/AirCombatState.cpp#L866)).
- The **primary UFO enters at position 7**; **escort UFOs enter at position 6**
  ([`addUfo`, AirCombatState.cpp:744-751](src/Geoscape/AirCombatState.cpp#L744)).
- Moving toward the enemy **raises** your position; the UFO closing on you **lowers**
  its position. Two units are at the same range whenever the gap between their
  numbers is within a weapon's `range`.
- The screen shows a window of **9 columns** (`VISIBLE_COLUMNS = 9`, `COLUMN_WIDTH =
  40`px) and scrolls/interpolates the view as units move
  ([AirCombatState.h:251-254](src/Geoscape/AirCombatState.h#L251)).

### Standoff rules

- A unit at **position 0 cannot be attacked by an enemy that is also at 0** — the
  standoff row is a safe lobby until someone commits
  ([AirCombatState.cpp:1411](src/Geoscape/AirCombatState.cpp#L1411)).
- **Movement is blocked by occupancy:** the player can't move forward onto/over a
  tile at or before an enemy's position, and the UFO can't move back onto/over a
  player's position (`STR_MOVE_BLOCKED_BY_ENEMY`,
  [AirCombatState.cpp:1345-1381](src/Geoscape/AirCombatState.cpp#L1345)). Units pass
  *through* the gap by maneuvering, not by stacking.
- Backward movement below position 0 is rejected (`STR_CANT_MOVE_BACKWARD`).

---

## 3. Combatants

Each side fields up to **4 units** (`MAX_UNITS = 4`): your interceptors on one side,
the primary UFO plus its escorts on the other. Both kinds of combatant are wrapped
in an `AirCombatUnit` ([AirCombatState.h:53](src/Geoscape/AirCombatState.h#L53))
that exposes a uniform interface — health, combat fuel, position, weapons — over the
underlying `Craft` or `Ufo`.

| Property | Player craft | UFO |
|----------|--------------|-----|
| **Health** | `damageMax − damage` of the craft | `damageMax − damage` of the UFO |
| **Combat fuel** | craft fuel **above the return-home reserve** (`getFuel() − getFuelLimit()`) | effectively infinite (returns `1000`) |
| **Weapons** | the craft's mounted `CraftWeapon`s (up to 4) | new `weapons:` list, **or** legacy `weaponPower`/`weaponRange`/`weaponAccuracy` |
| **AI** | none (player-controlled) | one `AirCombatAI` per UFO |
| **Speed handicap** | can be "too slow" to engage (see §8) | n/a |

Health is read live off the real `Craft`/`Ufo` damage values, so damage taken in the
minigame is the same damage the geoscape sees afterward
([`getHealth`, AirCombatState.cpp:305](src/Geoscape/AirCombatState.cpp#L305)).

---

## 4. Time & the turn queue

There is no "round." Instead every unit carries a **`turnDelay`** counter and the
unit with the **lowest delay acts next** — a classic time-unit / initiative queue
([`updateTurnQueue`, AirCombatState.cpp:2242](src/Geoscape/AirCombatState.cpp#L2242)).

How it flows:

1. **Seeding.** At combat start each unit gets a small staggered starting delay so
   they don't all act at once — player craft first (0, 1, 2 …), then enemies
   ([`setupCombat`, AirCombatState.cpp:1948](src/Geoscape/AirCombatState.cpp#L1948)).
2. **Pick the actor.** The queue is sorted ascending by `turnDelay`; the front unit
   is the current actor ([`getCurrentUnit`, AirCombatState.cpp:2324](src/Geoscape/AirCombatState.cpp#L2324)).
3. **Act.** The actor performs exactly one action. The action's **Time cost is added
   to its `turnDelay`** ([`spendTime`, AirCombatState.cpp:379](src/Geoscape/AirCombatState.cpp#L379)),
   pushing it back in line. A cheap action (e.g. Hold, 15) comes back around sooner
   than an expensive one (e.g. Pursue, 40).
4. **Re-normalize.** The smallest delay in the queue is subtracted from everyone so
   the front is always 0; the displayed delay numbers are therefore *relative* "turns
   until this unit acts" ([AirCombatState.cpp:2271-2295](src/Geoscape/AirCombatState.cpp#L2271)).
5. **Drop the empty.** Any unit whose combat fuel has reached 0 is removed from the
   queue and the fight (`updateTurnQueue` prunes them up front).

Up to **7 entries** of the queue are shown on the right of the screen
(`MAX_TURN_UNITS = 7`), each labeled with the unit's name and its relative delay,
color-coded for friend vs. UFO.

> **Design consequence:** faster/cheaper actions let a unit act more often. Holding
> position is the cheapest way to "win initiative," firing and pursuing are the most
> expensive. This is the lever the whole game balances on.

---

## 5. The combat-fuel economy

Maneuvering costs fuel, and fuel is the soft clock that ends fights.

- A player craft's **combat fuel is whatever fuel it has *beyond the amount it needs
  to fly home*** (`getFuel() − getFuelLimit()`,
  [AirCombatState.cpp:338](src/Geoscape/AirCombatState.cpp#L338)). You can't burn into
  your return reserve, so a craft far from base brings less fight to the table. A
  craft that arrives with **no spare fuel can't even join** the combat
  ([`addCraft`, AirCombatState.cpp:837](src/Geoscape/AirCombatState.cpp#L837)).
- **Every action's Energy cost is debited from that fuel.** Movement actions have a
  base Energy cost, and for player craft an **additional fuel-per-Time amount** is
  added on top via `Craft::getFuelPerTU(Time)`
  ([`getActionCost`, AirCombatState.cpp:294-297](src/Geoscape/AirCombatState.cpp#L294)),
  so slower/thirstier craft pay more to do the same maneuver.
- When a craft's combat fuel hits 0 it **drops out of the fight** (and, on the
  geoscape, will be heading home on its reserve).
- **UFOs have effectively unlimited combat fuel** (`getCombatFuel()` returns a flat
  `1000` and `spendCombatFuel` is a no-op for UFOs). The UFO never runs dry — but the
  AI still reasons about *your* fuel when deciding whether it can escape (§10).

This is the strategic heart of the redesign: the **player is on a fuel timer, the
UFO is not.** You must end the fight (kill, crash, or drive off the UFO) before your
spare fuel runs out, which discourages indefinite long-range plinking.

---

## 6. Actions

On its turn a player unit chooses from three action-menu groups, opened by the
**Move / Attack / Wait** buttons (or hotkeys; the **Special** button is reserved but
empty). Each menu lists its options with Time, fuel, ammo, accuracy, and range
readouts ([`AirActionMenuState`, constructor at AirActionMenuState.cpp:65](src/Geoscape/AirActionMenuState.cpp#L65)).

### Action catalog

| Action | Menu | Position change | Time | Energy (base) | Notes |
|--------|------|-----------------|------|---------------|-------|
| **Forward** (`AA_MOVE_FORWARD`) | Move | +1 toward enemy | 30 | 10 | Blocked if an enemy occupies the target tile; blocked if "too slow" |
| **Pursue** (`AA_MOVE_PURSUE`) | Move | +2 toward enemy | 40 | 30 | Fast close; expensive in time *and* fuel |
| **Backward** (`AA_MOVE_BACKWARD`) | Move | −1 away | 25 | 0 | Cheapest reposition; can't go below 0 |
| **Retreat** (`AA_MOVE_RETREAT`) | Move | −2 away | 30 | 10 | Fast disengage |
| **Hold** (`AA_HOLD`) | Wait | none | 15 | 0 | Cheapest action — best initiative |
| **Evade** (`AA_EVADE`) | Wait | none | 25 | 25 | Sets an evading flag for the unit |
| **Fire weapon** (`AA_FIRE_WEAPON`) | Attack | none | weapon's `tuAimed` (UFO: 20) | +fuel-per-Time | Costs 1 ammo; needs a target in range |

*(Base costs from [`getActionCost`, AirCombatState.cpp:249-298](src/Geoscape/AirCombatState.cpp#L249); for player craft, `Energy += getFuelPerTU(Time)` is added to all of the above.)*

- The **Attack menu** lists one entry per mounted craft weapon (up to 4, each on its
  own `keyAirWeapon1..4` hotkey); a UFO with only legacy `weaponPower` gets a single
  generic "UFO attack" entry ([AirActionMenuState.cpp:77-113](src/Geoscape/AirActionMenuState.cpp#L77)).
- Firing is a **targeted** action: after picking the weapon you select an enemy unit;
  the shot only goes off if the target is within `range` (`STR_OUT_OF_RANGE`
  otherwise).
- An action is rejected if it can't be paid for: `STR_NOT_ENOUGH_FUEL` /
  `STR_NOT_ENOUGH_AMMO` ([`canSpendCost`, AirActionMenuState.cpp:476](src/Geoscape/AirActionMenuState.cpp#L476)).

---

## 7. Attack resolution

When a weapon fires ([`animateAttack`, AirCombatState.cpp:1588](src/Geoscape/AirCombatState.cpp#L1588)):

### Hit chance

```
accuracy  =  weapon accuracy            (or UFO weaponAccuracy)
          −  target's avoidBonus        (craft/UFO stat)
          +  attacker's hitBonus        (craft/UFO stat)
          −  target's pilot dodge bonus (player craft only, from pilots)
hit       =  RNG::percent(accuracy)
```

([AirCombatState.cpp:1655-1663](src/Geoscape/AirCombatState.cpp#L1655)). Pilot skill
therefore matters on both ends — a well-piloted interceptor both hits more (hit
bonus) and is harder to hit (dodge bonus). *(The code notes these bonuses are
currently additive and flags making them multiplicative as a TODO.)*

### Damage

```
rawDamage =  weapon damage             (or UFO weaponPower)
damage    =  RNG::generate(0, rawDamage)        // uniform roll, 0..raw
damage    =  max(0, damage − target armor)      // flat armor subtraction
```

([AirCombatState.cpp:1782-1786](src/Geoscape/AirCombatState.cpp#L1782)). Damage is a
**uniform roll from 0 to the weapon's rated damage**, then reduced by the target's
armor as a flat subtractor (so armor can fully negate weak hits). The result is
applied straight to the real craft/UFO damage total. *(A shield layer is stubbed but
not implemented — see Unfinished.)*

### Multi-shot, misses, ammo

- A weapon with `shotsAimed > 1` fires that many projectiles, **200 ms apart**, each
  rolled independently ([AirCombatState.cpp:1601-1609, 1743-1748](src/Geoscape/AirCombatState.cpp#L1601)).
- Each shot consumes **1 ammo**; firing with an empty weapon is blocked
  (`STR_NO_ROUNDS_LEFT`).
- A **miss** still animates: the projectile is sent flying off the edge of the
  battle window along its trajectory rather than striking the target
  ([AirCombatState.cpp:1670-1741](src/Geoscape/AirCombatState.cpp#L1670)).
- On a hit, an explosion sprite plays at the impact point and the appropriate hit
  sound fires (interceptor-hit / UFO-hit).

---

## 8. Speed & whether you can even engage

A UFO that badly out-runs your interceptor can't be brought to battle
([`AirCombatUnit` craft constructor, AirCombatState.cpp:224-242](src/Geoscape/AirCombatState.cpp#L224)):

- Let `combatSpeed` be the craft's max speed and `ufoSpeed` the UFO's.
- If **`ufoSpeed > combatSpeed × 1.5`** the craft is flagged **`tooSlow`** — it can
  never close, and any Forward/Pursue attempt is rejected with `STR_CANT_KEEP_UP`
  ([AirCombatState.cpp:1331](src/Geoscape/AirCombatState.cpp#L1331)).
- Between 100 % and 150 % of combat speed, a `combatSpeedCost` multiplier is computed
  to make closing progressively more expensive; at or below 100 % it scales down.
  *(The multiplier is computed and stored but only partially applied in the current
  build — see Unfinished.)*

Design intent: fast UFOs (small scouts) force you into faster/heavier interceptors or
let them go, mirroring stock OpenXcom's "can't catch it" outcome but as a positional
constraint rather than a hard speed gate.

---

## 9. Outcomes

The fight ends when **one side is empty** — all enemies gone, or all your craft gone
or withdrawn (`_checkEnd` with `_enemyCount == 0 || _craftCount == 0`,
[AirCombatState.cpp:1191](src/Geoscape/AirCombatState.cpp#L1191)). Per-unit outcomes,
resolved in [`update`, AirCombatState.cpp:1077-1189](src/Geoscape/AirCombatState.cpp#L1077):

- **UFO destroyed** — explosion, `STR_UFO_DESTROYED`, and **double score**
  (`ufo score × 2`) credited to the country and region it was over. Its escorts are
  released and any X-COM craft that were following it return to base.
- **UFO crash-lands** — `STR_UFO_CRASH_LANDS`, **single score** (`× 1`). Over water
  it's simply destroyed; over land it becomes a crash site with the usual 24–96-hour
  despawn timer, ready for a ground assault. The unit that landed the killing blow is
  recorded (`setShotDownByCraftId`).
- **Interceptor destroyed** — explosion, `STR_CRAFT_DESTROYED`; the craft is lost.
- **Interceptor out of fuel** — silently dropped from the fight to fly home on its
  reserve.
- **Disengage / break-off** — when the fight ends with the UFO still alive it resumes
  its geoscape course (`_ufo->move()`).

Crucially, because health/damage/fuel are the *real* geoscape values, a UFO you only
wounded flies on damaged, and a craft that spent its spare fuel goes home low — there
is no separate "combat result" reconciliation step.

---

## 10. Enemy AI

Every UFO runs its own `AirCombatAI` ([AirCombatAI.cpp](src/Geoscape/AirCombatAI.cpp)).
After a short 250 ms "thinking" delay the current enemy unit takes its turn
([`performAIAction`, AirCombatState.cpp:1224](src/Geoscape/AirCombatState.cpp#L1224)).

### Sizing itself up

At construction the AI computes its **ideal** and **maximum** attack ranges from its
weapons, picking the range of its **best damage-per-Time weapon** as ideal
([AirCombatAI.cpp:31-52](src/Geoscape/AirCombatAI.cpp#L31)). A UFO with no offensive
range at all is born wanting to run.

### Modes

The AI holds one of four moods (`AirAIMode`) and transitions between them each turn
in [`selectMode`, AirCombatAI.cpp:59](src/Geoscape/AirCombatAI.cpp#L59):

| Mode | Behavior | Entered when… |
|------|----------|---------------|
| `AAI_SNIPE` | Maneuver to a good firing range and shoot; back off if pressed | Default for an armed primary UFO |
| `AAI_BERSERKER` | Press the attack, close if needed, ignore self-preservation | An **escort** sees a threat to itself or to the UFO it guards; or a cornered UFO that *can't* escape |
| `AAI_ESCAPE` | Run — keep moving away | Unarmed; or wounded; or judging that it can outlast pursuit |
| `AAI_NONE` | initial | startup only |

The escort/primary distinction drives a lot of this: **escorts fight to protect the
primary** (turning berserker when the primary is threatened, fleeing only when the
primary has already run far ahead), while a **lone primary is more self-interested**
(snipes, then flees below 75 % health).

### The turn decision

[`performTurn`, AirCombatAI.cpp:123](src/Geoscape/AirCombatAI.cpp#L123) evaluates the
candidate actions (move toward, move away, attack) and the **estimated survival
time** of itself and its prey at the current and adjacent positions, then picks:

- **Snipe:** if it can escape, flee; else if closing kills the target faster than it
  endangers itself, attack; else reposition to where it lives longest while still
  threatening; else shoot if able.
- **Berserker:** attack if possible, otherwise close the distance. No retreat.
- **Escape:** move away.
- If nothing applies it **Evades** as a fallback.

### The numbers behind it

- **Damage-per-Time (`getAttackerDpt`)** — for a given range, the best weapon that can
  reach, scored as `damage × shotsAimed × accuracy / 100 / cost`
  ([AirCombatAI.cpp:374](src/Geoscape/AirCombatAI.cpp#L374)). Weapons out of range
  contribute 0, which is what makes *position* the controlling variable.
- **Estimated life (`getEstimatedLife`)** — `health ÷ incoming DPT` summed over all
  enemies; infinite if nothing can hurt it ([AirCombatAI.cpp:339](src/Geoscape/AirCombatAI.cpp#L339)).
- **Pursuit time (`getAttackerPursuitTime`)** — how long your craft can afford to
  chase, from *your* combat fuel divided by the cost of a pursue move
  ([AirCombatAI.cpp:408](src/Geoscape/AirCombatAI.cpp#L408)). This is where the AI
  exploits the fuel asymmetry: it reasons about **your** clock, not its own.
- **`canEscape`** — true when its (escape-adjusted, doubled) estimated life exceeds
  the (doubled) pursuit time ([AirCombatAI.cpp:437](src/Geoscape/AirCombatAI.cpp#L437)):
  i.e. *"can I survive longer than they can afford to chase me?"* If yes, it runs and
  wins the war of attrition by outlasting your fuel.

> **Design consequence:** a damaged UFO that you can't catch quickly will rationally
> flee and escape on your fuel timer. To force a kill you must out-range, out-position,
> or out-pace it before it decides the math favors running.

---

## 11. Escort UFOs

A UFO ruleset entry can name **`escorts:`** — other UFO types that spawn alongside it
([`addUfo` recursion, AirCombatState.cpp:755-758](src/Geoscape/AirCombatState.cpp#L755);
RuleUfo `escorts:`). Escorts:

- Enter one column ahead of the primary (position 6 vs. 7), forming a screen between
  you and the target.
- Run the **escort AI personality** (protect the primary; go berserker when it's
  threatened).
- Are **released** when the primary dies/crashes (`releaseEscort`), so killing the
  primary scatters its guards rather than forcing you to grind all of them.

This adds a wolfpack dimension absent from stock interception: a heavily escorted
battleship is a genuine multi-craft engagement, and you can choose to punch through
to the primary or peel the escorts first.

---

## 12. Armed UFOs

In stock OpenXcom only your craft mount discrete weapons; UFOs use scalar
`weaponPower`/`weaponRange`/`weaponAccuracy` stats. OpenXcomDX keeps that legacy path
but adds a real **`weapons:` list on `RuleUfo`** so UFOs can carry the same
`CraftWeapon` definitions your craft do ([RuleUfo `weapons:`](src/Mod/RuleUfo.cpp);
runtime `Ufo::getWeapons()`). When present, the UFO's weapons drive its menu entry,
range, ammo, multi-shot, and DPT exactly like a player weapon; when absent it falls
back to the legacy scalars. This is what lets the example ruleset give scouts
distinct plasma armaments (`STR_UFO_SMALL_SCOUT_PLASMA`, etc.).

---

## 13. Status & how to turn it on

The minigame is **present in the build but disabled by default**, and has been gated
three different ways over its development:

1. **Compile-time:** a `#define NEW_AIR_COMBAT` in
   [GeoscapeState.h:39](src/Geoscape/GeoscapeState.h#L39) selects the `AirCombatState`
   path instead of the classic real-time `DogfightState`.
2. **Data-disable (committed):** commit `ca328878d` disabled it for the shipped
   `xcomtd` mod by **renaming its data files** — `UFOs.rul` and `CraftWeapons.rul`
   became `*.rul_NewAirCombat`, so they no longer load and UFOs receive no
   weapons/escorts/combat sprites. The leftover
   [CraftWeapons.rul_NewAirCombat](bin/standard/xcomtd/Ruleset/CraftWeapons.rul_NewAirCombat)
   and `UFOs.rul_NewAirCombat` still sit in the tree as the reference data.
3. **Runtime flag (working tree):** the gate was reworked into a real **ruleset
   option `enableNewAirCombat`** (`Mod::_enableNewAirCombat`, default **`false`**).
   `GeoscapeState` now keeps **two** interception queues — the classic
   `_dogfights` and the new `_newDogfights` — and routes each interception to one or
   the other based on the flag.

**To actually play it** you need to (a) compile with `NEW_AIR_COMBAT` defined,
(b) set `enableNewAirCombat: true` in a ruleset, and (c) provide the air-combat data:
UFO `weapons:`/`combatSprite`, craft `combatSprite`, the `AirCombatSprites` surface
set, and the `aircombat` interface element — i.e. re-enable the `*.rul_NewAirCombat`
content.

---

## 14. Ruleset & assets reference

### `RuleUfo`
| Key | Meaning |
|-----|---------|
| `weapons:` | list of `CraftWeapon` types the UFO carries into air combat |
| `escorts:` | list of UFO types spawned as escorts alongside this UFO |
| `combatSprite` | frame index into the `AirCombatSprites` surface set |
| *(legacy)* `power`/`range`/`accuracy` | fallback scalar weapon when `weapons:` is empty |

### `RuleCraft`
| Key | Meaning |
|-----|---------|
| `combatSprite` | frame index into `AirCombatSprites` for this craft |

### `CraftWeapon` fields the minigame reads
`damage`, `range`, `accuracy`, `tuAimed` (→ fire Time cost), `shotsAimed`
(multi-shot), `ammoMax`/ammo, `bulletSprite`, `bulletSpeed`, `bulletParticles`,
`sound`.

### Mod / option
| Key | Default | Meaning |
|-----|---------|---------|
| `enableNewAirCombat` | `false` | Route interceptions to the new minigame |

### Assets & interface
- Surface set **`AirCombatSprites`** (per-craft/UFO combat sprites) with fallbacks
  `AirCombatUnknownCraft` / `AirCombatUnknownUfo`.
- Background surface **`AirCombatScreen`**; projectile/hit/explosion use
  `Projectiles`, `SMOKE.PCK`, `X1.PCK`.
- Interface block **`aircombat`** (elements: `actionButton`, `unitName`,
  `barHealth`, `barEnergy`, `turnUnit`, `hoverUnit`, `currentUnit`,
  `minimizedNumber`).
- Keybindings: `keyAirMove`, `keyAirAttack`, `keyAirSpecial`, `keyAirWait`,
  `keyAirWeapon1..4`.

### Example data (from the disabled reference ruleset)
```yaml
# UFOs.rul_NewAirCombat
ufos:
  - type: STR_MEDIUM_SCOUT
    combatSprite: 102
    damageMax: 200
    speedMax: 2400
    weapons:
      - STR_UFO_SMALL_SCOUT_PLASMA
      - STR_UFO_MEDIUM_SCOUT_PLASMA
    escorts:
      - STR_SMALL_SCOUT

# CraftWeapons.rul_NewAirCombat
craftWeapons:
  - type: STR_CANNON_UC
    damage: 10
    range: 2          # short range — must close
    accuracy: 50
    tuAimed: 10       # cheap to fire → frequent shots
    shotsAimed: 5     # 5 projectiles per burst
    ammoMax: ...
```

---

## 15. UI layout

```
┌──────────────────────────────────────────────────────────┐
│  combat log (scrolling events)                           │
│                                                          │
│        [escort]      [UFO]                               │  ← range axis,
│   [your craft]                                           │     9 visible columns
│                                                          │
├───────────────┬──────────────────────┬───────────────────┤
│ craft 1 name  │   [ MOVE  ]          │ turn queue:       │
│ ▆▆▆ health  │   [ ATTACK]          │  0  UFO           │
│ ▆▆▆ fuel    │   [SPECIAL]          │  2  your craft    │
│ craft 2 …     │   [ WAIT  ]          │  5  escort     …  │  ◣ minimize (308,188)
└───────────────┴──────────────────────┴───────────────────┘
```

- **Left:** one name + health bar + fuel bar per friendly craft (up to 4).
- **Center:** the four action buttons (Move / Attack / Special / Wait).
- **Right:** the initiative queue (up to 7 entries, relative delay + name, colored
  friend/UFO).
- **Top:** the floating combat log (shared with the battlescape combat-log widget),
  reporting moves, fire, damage, and kills; damage is shown numerically only for
  **researched** UFO types (otherwise high/low/no-damage prose).
- **Minimize** button (bottom-right) collapses the fight to the standard dogfight
  icon so multiple simultaneous interceptions can run minimized, with the
  interception number shown on the icon — same UX as stock multi-dogfight handling.

---

## 16. Unfinished / rough edges

The system works end-to-end but carries visible work-in-progress; weigh these before
relying on or balancing around it:

- **Off by default** and dependent on hand-authored data/sprites (§13).
- **Special actions** are unimplemented — the menu group is empty.
- **UFO weapon Time cost** is hardcoded to 20 when a UFO fires (a `// TODO: Handle TU
  cost for UFO weapons` sits right there, [AirCombatState.cpp:288](src/Geoscape/AirCombatState.cpp#L288)).
- **`combatSpeedCost`** (the 100–150 % speed handicap) is computed but only partially
  applied; movement costs are otherwise flat constants rather than speed/fuel-derived
  as the `// TODO: Use Speed or Fuel Consumption` note intends.
- **Accuracy bonuses are additive**, flagged for conversion to multiplicative.
- **Shield damage** is a `// TODO` with no implementation
  ([AirCombatState.cpp:1788](src/Geoscape/AirCombatState.cpp#L1788)).
- The **hit sprite** is hardcoded (`26`) rather than read from the weapon.
- Large blocks of alternative logic (per-turn fuel drain on `turnDelay`, standoff
  firing restrictions, enemy auto-move) are present but **commented out**, so the
  shipped behavior is the simpler subset described above.
- The `XY::operator=` returns nothing (missing `return *this;`) — harmless in current
  use but technically UB.

---

## 17. Code map

| Area | File | Key symbols |
|------|------|-------------|
| Combat screen, turn loop, resolution | [AirCombatState.cpp](src/Geoscape/AirCombatState.cpp) | `AirCombatState`, `AirCombatUnit`, `update`, `updateTurnQueue`, `executeAction`, `animateAttack`, `onProjectileHit` |
| Enemy AI | [AirCombatAI.cpp](src/Geoscape/AirCombatAI.cpp) | `AirCombatAI`, `selectMode`, `performTurn`, `getEstimatedLife`, `getAttackerDpt`, `canEscape` |
| Action menu & costs | [AirActionMenuState.cpp](src/Geoscape/AirActionMenuState.cpp) | `AirActionMenuState`, `AirCombatAction`, `addItem`, `updateCost`, `spendCost` |
| Geoscape integration & gate | [GeoscapeState.cpp](src/Geoscape/GeoscapeState.cpp) / [.h](src/Geoscape/GeoscapeState.h) | `#define NEW_AIR_COMBAT`, `_newDogfights`, `_enableNewAirCombat` routing |
| Ruleset | [RuleUfo.cpp](src/Mod/RuleUfo.cpp), [RuleCraft.cpp](src/Mod/RuleCraft.cpp), [Mod.cpp](src/Mod/Mod.cpp) | `weapons`/`escorts`/`combatSprite`, `enableNewAirCombat` |
| Reference data | [bin/standard/xcomtd/Ruleset/](bin/standard/xcomtd/Ruleset/) | `UFOs.rul_NewAirCombat`, `CraftWeapons.rul_NewAirCombat`, `Crafts.rul` |
