# Ruleset: `MCDPatches:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`MCDPatch`](../src/Mod/MCDPatch.h) · **List key:** `type` (a **map data set**
name) · **Loader:** [`MCDPatch::load`](../src/Mod/MCDPatch.cpp) (the root itself is read in
[`Mod::loadFile`](../src/Mod/Mod.cpp))

An MCD patch **overwrites individual properties of individual tile records** inside a binary `.MCD`
file, without touching the file. The patch is keyed by the
[map data set](Ruleset-Terrains.md#map-data-sets-mapdatasets) name (the MCD file), and each `data:`
entry targets one tile record by its `MCDIndex` within that set. Patches are applied whenever the
data set is loaded for a battle
([`MapDataSet::loadData`](../src/Mod/MapDataSet.cpp) via `Mod::getMCDPatch`).

This exists to fix bugs in the original game's terrain data (a wall that does not block line of
sight, a floor that costs the wrong TUs). The engine's own copy of this root carries the comment
*"this should not be used by mods"* — prefer shipping corrected MCD files, and reach for a patch only
when you must fix a data set you do not own.

```yaml
MCDPatches:
  - type: U_WALL02              # the MCD / mapDataSet name
    data:
      - MCDIndex: 34
        stopLOS: true           # this wall really should block sight
        armor: 40
      - MCDIndex: 35
        bigWall: 1
        LOFTS: [ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 ]
```

Unlike most roots, MCD patches are **additive**: loading a patch for a `type` that already has one
appends the new tweaks to the existing list rather than replacing it. There is no `refNode:` and no
`delete:` support.

## Patch entries (`data:`)

Each entry needs `MCDIndex`; every other key is optional and patches exactly the field it names.
Fields not listed here cannot be patched — they can only be changed in the MCD file itself.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `MCDIndex` | int | — | **Required.** The tile record's index within this map data set. |
| `objectType` | int | — | Which tile part the record is (`TilePart`: 0 floor, 1 west wall, 2 north wall, 3 object). |
| `specialType` | int | — | Special tile type (`SpecialTileType` in [MapData.h](../src/Mod/MapData.h): 0 none, 1 start point, 2 UFO power source, …, 13 end point, 14 must-destroy). |
| `bigWall` | int | — | Big-wall type ([`Pathfinding`](../src/Battlescape/Pathfinding.h): 0 none, 1 block, 2 NESW diagonal, 3 NWSE diagonal, 4 west, 5 north, 6 east, 7 south, 8 east+south, 9 west+north). |
| `TUWalk` | int | — | TU cost to walk onto/through this tile part (−1 = impassable). |
| `TUFly` | int | — | TU cost when flying. |
| `TUSlide` | int | — | TU cost when sliding. |
| `armor` | int | — | Tile part's armor (how much damage it takes to destroy). |
| `explosive` | int | — | Explosive power released when the tile part is destroyed. |
| `flammability` | int | — | How easily it catches fire (lower = more flammable). |
| `fuel` | int | — | How long it keeps burning once lit. |
| `HEBlock` | int | — | How much high-explosive blast this tile part blocks. |
| `deathTile` | int | — | MCD index of the record this one turns into when destroyed. |
| `terrainHeight` | int | — | Terrain level (vertical offset of the surface, in voxels; negative = higher). |
| `footstepSound` | int | — | Footstep sound index for this surface. |
| `noFloor` | bool | — | True = units fall through (this record is not a floor). |
| `stopLOS` | bool | — | True = this tile part blocks line of sight. |
| `LOFTS` | list of ints | — | The 12 LOFT template ids (bottom to top) that form the tile part's 3-D collision silhouette. |

## See also

- [`terrains:`](Ruleset-Terrains.md) — the `mapDataSets:` this root patches, and the map blocks built on them
- [`mapScripts:`](Ruleset-MapScripts.md) — `tunnelData:` also references MCD set/entry pairs directly
