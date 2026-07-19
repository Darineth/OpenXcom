# Ruleset: `startingBase:` (and the per-difficulty variants)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`Base`](../src/Savegame/Base.h) (consumer) · **Node type:** singleton (a map, not a
list) · **Loader:** [`Mod::loadFile`](../src/Mod/Mod.cpp) stores the node verbatim;
[`Mod::newSave`](../src/Mod/Mod.cpp) replays it through
[`Base::load`](../src/Savegame/Base.cpp) when a campaign starts.

`startingBase:` is the **template of the player's first base**: what is already built, what is in
the stores, which craft sit in the hangars, and how many soldiers/scientists/engineers X-COM starts
with. It is unusual among the roots: the engine does not parse it into `Rule*` objects at mod-load
time — it **keeps the YAML sub-tree as text** and feeds it to the savegame loader when you press
"New Game". That means the node's shape is exactly a **saved base's** shape, not a bespoke schema.

```yaml
startingBase:
  facilities:
    - type: STR_ACCESS_LIFT
      x: 2
      y: 2
    - type: STR_LIVING_QUARTERS
      x: 3
      y: 2
  crafts:
    - type: STR_SKYRANGER
      id: 1
      fuel: 1500
      items:
        STR_RIFLE: 6
        STR_RIFLE_CLIP: 12
      status: STR_READY
  items:
    STR_PISTOL: 2
    STR_PISTOL_CLIP: 8
  randomSoldiers: 8
  scientists: 10
  engineers: 10
```

(The stock version lives in
[`bin/standard/xcom1/startingBase.rul`](../bin/standard/xcom1/startingBase.rul).)

## The six roots

| Root | Used when |
|---|---|
| `startingBase:` | Always — the fallback/default template. |
| `startingBaseBeginner:` | Difficulty 0, **if defined**; otherwise `startingBase:`. |
| `startingBaseExperienced:` | Difficulty 1, if defined. |
| `startingBaseVeteran:` | Difficulty 2, if defined. |
| `startingBaseGenius:` | Difficulty 3, if defined. |
| `startingBaseSuperhuman:` | Difficulty 4, if defined. |

Selection is [`Mod::getStartingBase(diff)`](../src/Mod/Mod.cpp): a per-difficulty node is used only
if it is non-empty; there is **no field-by-field merge with `startingBase:`** — a difficulty variant
you define must be *complete*.

**Merging across mods** works differently here than for keyed lists: each root is a single YAML
document fragment, and a later mod re-declaring the root **deep-merges its keys into the stored
template** (the node is re-emitted over the previous one). A later mod that redefines `facilities:`
replaces that list wholesale; keys it doesn't mention survive.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `facilities` | list | — | Pre-built [facilities](Ruleset-Facilities.md): each entry is `{ type, x, y }` (grid coordinates, 0-based, in the 6×6 base grid) plus any `BaseFacility` save fields (e.g. `buildTime`). |
| `crafts` | list | — | Craft in the hangars: each entry is a saved [craft](Ruleset-Crafts.md) — `type`, `id`, `fuel`, `damage`, `status`, `weapons: [{ type, ammo }]`, and an `items:` map of its onboard equipment. |
| `items` | map item → qty | — | Base stores contents. |
| `soldiers` | list | — | **Explicit** soldiers (a saved-soldier node each: `type`, `name`, `initialStats`, `currentStats`, …). Rarely used — most mods use `randomSoldiers` instead. |
| `randomSoldiers` | int **or** map | — | Soldiers generated with random names/stats. An **int** = that many, drawn from all [soldier types](Ruleset-Soldiers.md) with no `requires:`; a **map** of `SOLDIER_TYPE: count` = exactly those types. |
| `scientists` | int | 0 | Idle scientists at the base. |
| `engineers` | int | 0 | Idle engineers at the base. |
| `lon` / `lat` | float radians | — | Base location (usually omitted — the player picks it). Inherited from the `Target` save node. |
| `name` | string | — | Base name (usually omitted — the player types it). |
| `transfers` | list | — | Items/personnel already in transit at game start. |
| `research` / `productions` | lists | — | Research projects / production lines already running. |
| `globalTemplates` | list | — | Pre-seeded equipment templates (the inventory "save template" slots), loaded via `SavedGame::loadTemplates`. |
| `ufopediaRuleStatus` | map | — | Pre-set UFOpaedia article read/unlocked states. |

Anything else `Base::load` understands (`fakeUnderwater`, `retaliationTarget`, …) is technically
accepted, but only the keys above are meaningful for a fresh campaign.

## What the engine does with it

Read [`Mod::newSave`](../src/Mod/Mod.cpp) for the full sequence; the parts a modder must know:

1. **`facilities:` is skipped when the player enables "custom initial base"** (`Options::customInitialBase`).
   In that mode the engine instead calls
   [`Mod::getCustomBaseFacilities`](../src/Mod/Mod.cpp), which walks the same `facilities:` list and
   hands the player everything **except the access lift and upgrade-only facilities** to place by
   hand. So the list still defines *what* you get, just not *where*.
2. **Craft weapons are stripped** from a craft only when its *effective* soldier or vehicle
   capacity (rule stats plus installed weapon-module bonuses) is **negative** — i.e. a weapon
   module's stat penalty drove it below zero; the launcher + clips are refunded into base stores.
   A plain `soldiers: 0` interceptor keeps its weapons.
3. **`randomSoldiers` soldiers are generated, then auto-assigned to craft**: pilots fill
   interceptors up to their `pilots:` requirement first, everyone else goes to the first transport
   with room. 2×2 ("large") soldiers stay in the base.
4. If the mod defines the `STR_MEDAL_ORIGINAL8_NAME` [commendation](Ruleset-Commendations.md), the
   soldiers generated from `randomSoldiers` are awarded it (marked "old", so it doesn't pop a
   notification) — soldiers listed explicitly under `soldiers:` are not.
5. Craft and soldier **ids are re-registered** with the savegame, so `id:` values in the template
   only need to be unique within it.

## Gotchas

- A `randomSoldiers: N` with **no eligible soldier type** (all of them gated behind `requires:`)
  logs an error and starts you with zero soldiers.
- `facilities:` must include an access lift (`isLift: true`) — the base is otherwise invalid, and in
  custom-base mode the lift is the one facility the engine places for you.
- The node is stored as **text**, so a YAML error inside it surfaces as a
  "(starting base template)" parse failure at *New Game* time, not at mod-load time.
- Because the per-difficulty roots don't merge with the default, the usual pattern is: define
  `startingBase:` fully, and only add a `startingBaseSuperhuman:` etc. when a difficulty really
  needs a different loadout.

## See also

- [`facilities:`](Ruleset-Facilities.md) · [`crafts:`](Ruleset-Crafts.md) ·
  [`items:`](Ruleset-Items.md) · [`soldiers:`](Ruleset-Soldiers.md)
- [Ruleset-Globals.md](Ruleset-Globals.md) — `startingTime:`, `startingDifficulty:`,
  `initialFunding:` and the rest of the campaign-start knobs
