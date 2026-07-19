# Ruleset: `mapScripts:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`MapScript`](../src/Mod/MapScript.h) · **List key:** `type` · **Loader:**
[`MapScript::load`](../src/Mod/MapScript.cpp) (one call per entry of `commands:`; the root itself is
read in [`Mod::loadFile`](../src/Mod/Mod.cpp)) · **Executor:**
[`BattlescapeGenerator::generateMap`](../src/Battlescape/BattlescapeGenerator.cpp)

A map script is a **named list of commands** that assembles a battlefield out of the map blocks of a
[terrain](Ruleset-Terrains.md). The generator works on a grid of **10×10-tile cells**: every
coordinate, rect and `size:` in a map script is measured in cells, not tiles. Commands run **in
order**, each one placing, checking or removing blocks, until every cell of the map is filled — if
any cell is still empty when the script ends, map generation throws ("Map failed to fully generate").

The script to run is chosen by the [terrain](Ruleset-Terrains.md) (`script:`/`mapScripts:`) or, with
priority, by the [`alienDeployments:`](Ruleset-AlienDeployments.md) entry (`script:`/`mapScripts:`).

```yaml
mapScripts:
  - type: FOREST_SCRIPT
    commands:
      - type: resize                # 60x60 map, 4 levels
        size: [6, 6, 4]
      - type: addCraft              # drop the X-COM craft somewhere in the middle
        rects: [[1, 1, 4, 4]]
        label: 1
      - type: addUFO                # try to place the UFO
        canBeSkipped: false         # ...and abort generation if it can't be placed
        label: 2
      - type: addLine               # a road across the map
        direction: both
      - type: fillArea              # everything else = generic filler blocks
        groups: 0
```

Entries are keyed by `type`. Redefining a `type` in a later mod/file **replaces the whole command
list**; `delete: SCRIPT_NAME` removes it. There is no `refNode:` for map scripts.

## The command list (`commands:`)

Every entry needs a `type:` naming one of the commands below (an unknown name is a load error).

| Command | What it does |
|---|---|
| `addBlock` | Places **one** block (picked from `groups`/`blocks`) at a random free position inside `rects`. |
| `addLine` | Draws a road/line of blocks across the map in `direction`, using the road groups. |
| `addCraft` | Places the player's craft map (from the craft's own terrain), plus the blocks under it. |
| `addUFO` | Places a UFO map. Multiple UFOs can be added, but they must all share one MCD set. |
| `digTunnel` | Carves connections between already-placed blocks (used for base/module maps), replacing walls/floors per `tunnelData`. |
| `fillArea` | Repeats `addBlock` until no more blocks fit inside `rects` — the usual "fill the rest of the map" command. |
| `checkBlock` | Places nothing; succeeds if a block (optionally of a given group/index) already exists inside `rects`. A pure conditional. |
| `removeBlock` | Deletes the blocks inside `rects` (optionally filtered by group/index), freeing the cells again. |
| `resize` | Changes the map dimensions. Must run **before any block is placed**, and is illegal on base-defense maps. |

### Success, labels & conditionals

Each command records whether it **succeeded**. `label:` gives a command an id that later commands can
test through `conditionals:`: positive = "that command must have succeeded", negative = "must have
failed" (`conditionals: [1, -2]` = command 1 succeeded **and** command 2 failed). Referencing a label
that has not been defined *earlier* in the script is a fatal error, as is reusing a label.

An unlabelled (`label: 0`, the default) `addCraft` throws a generation exception if placement
fails — the assumption being that a labelled command's failure is handled by a follow-up command.
An unlabelled failed `addUFO` **logs and continues by default** (`canBeSkipped` defaults to true);
set `canBeSkipped: false` to make the failure abort generation instead. `addCraft` ignores
`canBeSkipped` entirely. `digTunnel` always reports success.

## Command parameters

All parameters are optional unless stated otherwise; unused ones are simply ignored by a command that
does not read them.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | **Required.** One of the commands in the table above. |
| `rects` | list of `[x, y, w, h]` | whole map | Areas (in 10-tile cells) the command applies to; positions are picked randomly inside them. |
| `size` | int or `[x, y, z]` | `1, 1, 0` (`resize`: `0, 0, 0`) | Block footprint the command asks for, in cells (a bare int sets both X and Y); for `resize` it is the new map size (X, Y in cells, Z in levels; 0 = leave that axis alone). |
| `groups` | int or list of ints | `[0]` (`addCraft`/`addUFO`: `[1]`) | Map-block [group](Ruleset-Terrains.md#block-groups) numbers to pick a block from. |
| `blocks` | int or list of ints | — | Pick blocks by **index into the terrain's `mapBlocks:` list** instead of by group (overrides `groups`). |
| `freqs` | int or list of ints | 1 each | Relative weight of each `groups`/`blocks` entry when picking. |
| `maxUses` | int or list of ints | −1 each | How many times each `groups`/`blocks` entry may be picked in this command (−1 = unlimited); an exhausted entry is removed from the pool. |
| `executions` | int | 1 | How many times the command body runs (each run re-rolls positions/blocks). |
| `executionChances` | int % | 100 | Chance the command runs at all (rolled once, before `executions`). |
| `label` | int | 0 | Id for this command, referenced by later `conditionals:` (negatives are made positive; 0 = unlabelled). |
| `conditionals` | int or list of ints | — | Only run if the labelled commands succeeded (positive) / failed (negative). |
| `craftGroups` | list of ints | — | Only run if the deployed craft belongs to one of these craft groups (see [`crafts:`](Ruleset-Crafts.md) `groups:`); ignored when no craft is deployed. |
| `direction` | `vertical`/`horizontal`/`both` | none | **Required for `addLine` and `digTunnel`.** Only the first letter matters (`V`/`H`/`B`, case-insensitive). |
| `verticalGroup` | int | 3 (N-S road) | `addLine`: block group used for the north-south leg. |
| `horizontalGroup` | int | 2 (E-W road) | `addLine`: block group used for the east-west leg. |
| `crossingGroup` | int | 4 (crossing) | `addLine`: block group used where the two legs meet. |
| `tunnelData` | map | — | `digTunnel`: what to carve; see [below](#tunneldata-digtunnel). |
| `UFOName` | string | — | `addUFO`: the [`ufos:`](Ruleset-Ufos.md) type whose map to place (default: the actual UFO of the mission, or the deployment's `customUfo`). |
| `craftName` | string | — | `addCraft`: force a specific [`crafts:`](Ruleset-Crafts.md) type's map instead of the player's actual craft (a [starting condition](Ruleset-StartingConditions.md) `craftTransformations:` can still override this). |
| `terrain` | string | mission terrain | Take the blocks for this command from another [terrain](Ruleset-Terrains.md) (clears `randomTerrain`). |
| `randomTerrain` | list of strings | — | Pool of terrains; one is rolled per execution (overrides `terrain`). |
| `canBeSkipped` | bool | true | `addUFO` only: on failed unlabelled placement, log and continue (the default); `false` aborts map generation instead. `addCraft` ignores it (always aborts). |
| `markAsReinforcementsBlock` | bool | false | Mark every cell this command fills as a valid spawn cell for [reinforcement waves](Ruleset-AlienDeployments.md#reinforcements-reinforcements) whose `mapBlockFilterType` includes the map-script filter (1, 3 or 4). |
| `verticalLevels` | list of maps | — | Stack several blocks in Z on the same cells; see [below](#vertical-levels-verticallevels). |

Two special terrain names may appear in `terrain:`/`randomTerrain:` (and in a vertical level's
`terrain:`): **`globeTerrain`** — the terrain implied by the globe texture under the mission — and
**`baseTerrain`** — the terrain of the X-COM base's globe texture.

### `tunnelData` (`digTunnel`)

```yaml
      - type: digTunnel
        direction: both
        tunnelData:
          level: 0                  # Z level the tunnel is carved on
          MCDReplacements:
            - type: westWall        # westWall | northWall | corner | floor
              set: 1                # index into the terrain's mapDataSets
              entry: 20             # MCD entry within that set
```

`level` (default 0) is the Z level to drill through. `MCDReplacements` lists which tile parts to
substitute where the tunnel breaks through: the recognized `type` values are **`westWall`**,
**`northWall`**, **`corner`** and **`floor`**; `set`/`entry` (both default −1 = "just remove the
existing part") point at a tile record in the terrain's [`mapDataSets`](Ruleset-Terrains.md#map-data-sets-mapdatasets).
If no `rects` are given, the drill uses a default `[3, 3, 3, 3]` corridor.

### Vertical levels (`verticalLevels:`)

A vertical level stacks additional blocks **on top of** the blocks the command places, so one command
can build a multi-storey structure (a cellar, several floors, a roof). Each entry:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | **required** | Level role: `ground`, `middle`, `ceiling`, `empty`, `decoration`, `craft`, `line`. An entry with no `type:` at all is **silently skipped**; a present-but-unrecognized value warns and loads as `middle`. |
| `size` | int or `[x, y, z]` | `1, 1, −1` (`decoration`: z = 0) | Footprint of this level's blocks in cells; z = −1 means "use the block's own height". |
| `maxRepeats` | int | −1 | How many times this level may repeat while filling the Z column (−1 = unlimited). |
| `groups` | int or list | — | Block groups to pick this level's block from. |
| `blocks` | int or list | — | Block indices to pick from (overrides `groups`). |
| `terrain` | string | command's terrain | Take this level's blocks from another terrain (also accepts `globeTerrain`/`baseTerrain`). |

The level types mean: **`ground`** = the bottom level, **`middle`** = repeated filler levels,
**`ceiling`** = the top level, **`empty`** = leave the Z range blank, **`decoration`** = overlay
blocks drawn into an existing level (height 0), **`craft`** = the level the `addCraft`/`addUFO` map
itself occupies, **`line`** = the level an `addLine` command draws on (an `addLine` with
`verticalLevels` **must** contain one).

## See also

- [`terrains:`](Ruleset-Terrains.md) — the map blocks, block groups and tile sets a script draws from
- [`alienDeployments:`](Ruleset-AlienDeployments.md) — selects the script, and the reinforcement waves that use `markAsReinforcementsBlock`
- [`ufos:`](Ruleset-Ufos.md) / [`crafts:`](Ruleset-Crafts.md) — the `battlescapeTerrainData` that `addUFO`/`addCraft` place
- [`MCDPatches:`](Ruleset-MCDPatches.md) — patching the tile records referenced by `tunnelData`
