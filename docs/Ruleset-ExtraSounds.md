# Ruleset: `extraSounds:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`ExtraSounds`](../src/Mod/ExtraSounds.h) · **List key:** `type` · **Loader:**
[`ExtraSounds::load`](../src/Mod/ExtraSounds.cpp)

`extraSounds:` adds or replaces entries in a named **sound set** — the CAT sound banks the engine
indexes by number: `BATTLE.CAT` (battlescape effects, weapons, unit voices), `GEO.CAT` (geoscape/UI),
`INTRO.CAT`. Every `*Sound:` field elsewhere in the rules (item `fireSound`, armor `deathMale`,
`interfaces:` `sound`, …) is just an index into one of these sets.

```yaml
extraSounds:
  - type: BATTLE.CAT
    files:
      55: Resources/MyMod/Sounds/laser_rifle.wav
      56: Resources/MyMod/Sounds/laser_rifle_hit.wav
      100: Resources/MyMod/Sounds/voices/    # folder: loads in natural order from index 100 up
```

Entries **accumulate** — every `extraSounds:` entry in every mod is applied in load order, so several
mods can extend the same set.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Name of the sound set to create/patch (normally `BATTLE.CAT`, `GEO.CAT` or `INTRO.CAT`). |
| `files` | map int → path | — | Sound index → audio file. A path ending in `/` is a **folder**: its files load in natural sort order starting at that index. |

That is the entire node — `ExtraSounds::load` reads nothing else.

## Index offsets (the part that bites)

Indices **at or above the set's shared-sound count** (the vanilla sound count of that CAT) are
automatically relocated into the **current mod's private index range**. So an index you write here is
a *mod-relative* number, and the same number used in an item's `fireSound:` resolves to the same
final slot — this is what stops two mods from overwriting each other's sounds. Indices **below** the
shared count deliberately overwrite the vanilla sound.

Going past the mod's index budget is a hard load error
(`ExtraSounds '<type>' sound '<n>' exceeds mod '<name>' size limit`). A file that fails to load from a
folder is logged and skipped rather than aborting.

Creating a **new**, previously unknown set is allowed but warned about — nothing in the engine will
look it up ("this will likely have no in-game use").

## See also

- [`extraSprites:`](Ruleset-ExtraSprites.md) — the image equivalent, with the same index/offset rules
- [`soundDefs:`](Ruleset-SoundDefs.md) — remapping which CAT the engine pulls a sound set from (TFTD)
- [`unitResponseSounds:`](Ruleset-UnitResponseSounds.md) — per-soldier voice banks built from these indices
- [`musics:`](Ruleset-Musics.md) — music tracks (a separate system from sound sets)
