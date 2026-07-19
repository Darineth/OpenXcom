# Ruleset: `terrains:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleTerrain`](../src/Mod/RuleTerrain.h) · **List key:** `name` · **Loader:**
[`RuleTerrain::load`](../src/Mod/RuleTerrain.cpp)

A terrain is a **battlescape tileset plus the pool of map blocks** built from it: which MCD tile
sets to load (`mapDataSets`), which map blocks exist and what groups they belong to (`mapBlocks`),
plus per-terrain flavor — music, ambient sound, civilians, depth, and which
[map script](Ruleset-MapScripts.md) assembles the blocks into a battlefield.
[`alienDeployments:`](Ruleset-AlienDeployments.md) (or the globe texture) select the terrain for a
mission; the map script then pulls blocks out of it.

```yaml
terrains:
  - name: FOREST
    mapDataSets:            # MCD tile sets, in order — indexing is cumulative
      - BLANKS
      - FOREST
    mapBlocks:
      - name: FOREST00      # FOREST00.MAP / FOREST00.RMP
        width: 10
        length: 10
      - name: FOREST01
        width: 20
        length: 20
        groups: [ 1 ]       # landing-zone block
    civilianTypes: [ MALE_CIVILIAN, FEMALE_CIVILIAN ]
    music: [ GMTACTIC ]
    ambience: 55
    script: FOREST_SCRIPT   # mapScripts: entry used to assemble the map
```

Entries **merge** across mods/files by `name`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Terrain fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; referenced from deployments, globe textures, craft/UFO `battlescapeTerrainData`, map script `terrain:` keys. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `mapDataSets` | list of strings | — | MCD tile set names to load, in order (see [Map data sets](#map-data-sets-mapdatasets) below). Redefining replaces the whole list. |
| `mapBlocks` | list of maps | — | The map block pool (see [Map blocks](#map-blocks-mapblocks) below). Redefining replaces the list unless `addOnly` is set. |
| `addOnly` | bool | false | If true, this entry's `mapBlocks` are **appended** to the inherited list instead of replacing it. |
| `script` | string | `DEFAULT` | Name of the [`mapScripts:`](Ruleset-MapScripts.md) entry used to generate the map (a deployment's own `script`/`mapScripts` takes priority in mission setup). |
| `mapScripts` | list of strings | — | Pool of map script names; one is picked at random per battle and overrides `script`. |
| `enviroEffects` | string | — | [`enviroEffects:`](Ruleset-EnviroEffects.md) rule applied on this terrain (the deployment's own `enviroEffects` wins if both are set). |
| `civilianTypes` | list of units | `[MALE_CIVILIAN, FEMALE_CIVILIAN]` | [Unit](Ruleset-Units.md) types spawned when the deployment asks for (untyped) civilians. |
| `music` | list of strings | — | Battlescape music tracks to pick from (deployment `music` wins if set). |
| `depth` | `[min, max]` | `[0, 0]` | Battle depth range (TFTD): 0 = surface, >0 = underwater; the actual depth is rolled between min and max (deployment `depth` overrides). |
| `ambience` | sound id | −1 (none) | Looping ambient sound (offset into `BATTLE.CAT`, mod offsets apply). |
| `ambientVolume` | float | 0.5 | Volume of the ambient loop (0.0–1.0). |
| `ambienceRandom` | list of sound ids | — | Pool of one-shot sounds played at random intervals during the battle. |
| `ambienceRandomDelay` | `[min, max]` | `[20, 60]` | Delay range between random ambient sounds, in seconds. |

## Map blocks (`mapBlocks:`)

Each entry is one **`MapBlock`** ([loader](../src/Mod/MapBlock.cpp)): a `.MAP` file (tile layout)
plus its `.RMP` (route nodes), both found by `name`. Blocks are the unit of map assembly — the
[map script](Ruleset-MapScripts.md) places them on a grid of 10×10-tile cells.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | The MAP/RMP file base name. (Map scripts select blocks by **index** into this list or by group — never by name.) |
| `width` | int | 10 | X size in tiles; **must be a multiple of 10** (load error otherwise). |
| `length` | int | 10 | Y size in tiles; must be a multiple of 10. |
| `height` | int | 4 | Z size in levels (must match the battle map height unless vertical levels stack blocks). |
| `groups` | int or list | `[0]` | Group number(s) the block belongs to — map scripts select blocks by group (see below). |
| `revealedFloors` | int or list | — | Z level(s) of this block that start revealed (no fog) at battle start. |
| `items` | map | — | `itemType: [[x,y,z], …]` — items spawned on specific tiles of the block. |
| `fuseTimers` | map | — | `itemType: [min, max]` — pre-primed fuse range applied to that item type spawned via `items`. |
| `randomizedItems` | list of maps | — | Random item drops; each entry: `position` `[x,y,z]`, `amount` (default 1), `itemList` (pool), `mixed` (false = one type picked for all, true = rolled per item), `fuseTimerMin`/`fuseTimerMax` (−1 = none). |
| `extendedItems` | list of maps | — | Extended item placement; each entry: `type`, `pos` (list of `[x,y,z]`), `fuseTimerMin`/`fuseTimerMax` (−1 = none), `ammoDef` (list of `[ammoType, quantity]` pairs — the ammo is created and, if compatible, loaded into the weapon; incompatible or double-loaded ammo is a generation error). Corpse-type items are rejected. |
| `craftInventoryTile` | `[x, y, z]` | — | Tile (block-relative) usable as the equipment-pile tile when no craft supplies one. |

### Block groups

Group numbers are how map scripts ask for "any block of kind X". 0–4 have engine meaning
(`MapBlockType` in [MapBlock.h](../src/Mod/MapBlock.h)); higher numbers are free for mod use:

| Group | Meaning |
|---|---|
| 0 | default / generic filler |
| 1 | landing zone (flat 10×10; what `addCraft`/`addUFO` place under the craft by default) |
| 2 | east-west road |
| 3 | north-south road |
| 4 | road crossing |

## Map data sets (`mapDataSets:`)

`mapDataSets` names **MCD files** (`MapDataSet` — tile-record sets with matching PCK/TAB
sprites). There is no separate ruleset node for them: naming one here creates/references it, and
the file contents are loaded lazily when a battle starts ([`Mod::getMapDataSet`](../src/Mod/Mod.cpp)).

- **Order matters.** A `.MAP` file addresses tiles by a running index across the terrain's data
  sets in list order; reordering or resizing sets breaks every block built against them. Invalid
  tile references fall back to entry 0 of the first set.
- By convention the first set is **`BLANKS`** (the empty-tile set) so index 0 is a blank.
- Tile properties in these files can be patched from the ruleset via
  [`MCDPatches:`](Ruleset-MCDPatches.md) (keyed by data set name).
- For craft skins, sets are re-pointed to `NAME_<skinIndex>` variants at generation time
  (`RuleTerrain::refreshMapDataSets`); `BLANKS` is exempt.

## See also

- [`mapScripts:`](Ruleset-MapScripts.md) — the commands that assemble the blocks into a map
- [`alienDeployments:`](Ruleset-AlienDeployments.md) — selects terrain, shade, depth and the roster
- [`MCDPatches:`](Ruleset-MCDPatches.md) — patching tile (MCD) properties per data set
- [`enviroEffects:`](Ruleset-EnviroEffects.md) — per-terrain environmental effects
- [`items:`](Ruleset-Items.md) — the item types spawned by `items:`/`randomizedItems:`/`extendedItems:`
