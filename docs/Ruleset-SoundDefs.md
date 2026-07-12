# Ruleset: `soundDefs:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`SoundDefinition`](../src/Mod/SoundDefinition.h) · **List key:** `type` · **Loader:**
[`SoundDefinition::load`](../src/Mod/SoundDefinition.cpp) (invoked from
[`Mod::loadResourceConfigFile`](../src/Mod/Mod.cpp))

`soundDefs:` **builds a sound set out of hand-picked entries of a CAT file**. It exists because TFTD's
audio catalogs are not laid out like UFO's: the engine expects a `GEO.CAT` and a `BATTLE.CAT` whose
indices mean specific things, and TFTD's `SAMPLE.CAT`/`SAMPLE2.CAT` simply don't line up. A
`soundDefs:` entry says "sound set `GEO.CAT` is: entry 120 of `SAMPLE.CAT`, then entry 98, then
entry 27, …", giving the engine the index layout it expects.

```yaml
soundDefs:
  - type: GEO.CAT            # the sound set the engine will ask for
    file: SAMPLE.CAT         # the real CAT file in SOUND/ to pull from
    sounds:
    # UI sounds: press and popup
      - 120
      - 98
    # ufo hit / crash / explode / fire
      - 27
      - 28
      - 31
      - 37
  - type: BATTLE.CAT
    file: SAMPLE2.CAT
    soundRanges:
      - [0, 54]              # inclusive [first, last] — expands to 0,1,2,...,54
    sounds:
      - 96
```

**This node is not read from ordinary `.rul` files.** It is only parsed from a mod's **resource config
file** — the file named by `resourceConfig:` in the mod's `metadata.yml` (TFTD uses
`resourceConfig: vars.rul`). Entries merge by `type`.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Name of the sound set to build (the name the engine and other rules index into: `GEO.CAT`, `BATTLE.CAT`, …). |
| `file` | string | — | The actual catalog file in the `SOUND/` folder to read the entries from (e.g. `SAMPLE.CAT`). |
| `soundRanges` | list of `[first, last]` | — | Inclusive index ranges in `file`, each expanded to every index in between. |
| `sounds` | list of ints | — | Individual indices in `file`. |

## How the mapping works

`soundRanges` are expanded first, then `sounds` are appended — the two lists are concatenated into
one ordered **sound list**. The set is then filled in that order: the *n*-th entry of the list becomes
**index *n* of the resulting sound set**. So the position in the YAML is the index the engine will
use, and the number written there is the index in the source CAT.

Merely defining **any** `soundDefs:` switches the engine out of its vanilla-UFO sound loading path
entirely (`GEO.CAT`/`BATTLE.CAT` are no longer auto-detected from `SOUND1/2.CAT` or
`SAMPLE/SAMPLE2.CAT`), and hides the DOS/Windows sound-format option in the audio menu, since the
format is now fully determined by the ruleset.

## See also

- [`extraSounds:`](Ruleset-ExtraSounds.md) — adding/replacing individual sounds in a set
- [`transparencyLUTs:`](Ruleset-TransparencyLUTs.md) — the other root parsed from the resource config file
- [`musics:`](Ruleset-Musics.md) — music tracks
