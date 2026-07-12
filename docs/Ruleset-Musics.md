# Ruleset: `musics:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleMusic`](../src/Mod/RuleMusic.h) · **List key:** `type` · **Loader:**
[`RuleMusic::load`](../src/Mod/RuleMusic.cpp)

A `musics:` entry declares one **music track**: the name the engine plays it by, where to find it in
the original Adlib/MIDI catalog, and how loud the Adlib version should be. Rules refer to tracks by
`type` (`interfaces:` `music:`, cutscene `musicId:`, `alienDeployments:` `music:`, the `GM*` names the
engine hardcodes for geoscape/battle themes, …).

```yaml
musics:
  - type: GMDEFEND        # the track id the engine plays by name
    catPos: 3             # index inside the vanilla music CAT
    normalization: 0.76
  - type: GMENBASE
    catPos: 6
    normalization: 0.83
  - type: GMGEO3          # no catPos: only an external GMGEO3.ogg/.mp3/... will play
```

Entries **merge** by `type`.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | The track id the engine plays by name; also the default filename looked for in the `SOUND` folder. |
| `name` | string | `type` | Override the on-disk filename (**no extension**) when it differs from the id — the engine tries the supported formats (OGG/MP3/MID/FLAC…) in the user's configured order. |
| `catPos` | int | `INT_MAX` (none) | Index of the track inside the vanilla music catalog (`GM.CAT` / `SAMPLE3.CAT`). Omit it for tracks that only exist as external files. |
| `normalization` | float | 0.76 | Volume normalization multiplier, applied to the **Adlib** mixer only. TFTD tracks are roughly optimal at 0.76. |

## Notes

- `catPos` defaulting to `INT_MAX` is deliberate: it means "not in a CAT", so a mod can list an
  optional track and have it play if (and only if) the player has an `.ogg`/`.mp3` version of it,
  without the engine trying to read a nonexistent catalog entry.
- Tracks are **grouped by name prefix**: when the engine asks for a random track of a family (e.g.
  `GMGEO`), it picks among every loaded track whose id *contains* that string — which is why the
  vanilla set defines `GMGEO1`…`GMGEO9`.
- Music is muted wholesale by the `mute` option; a muted game returns a dummy track and never touches
  these rules.

## See also

- [`interfaces:`](Ruleset-Interfaces.md) — `music:` per screen
- [`cutscenes:`](Ruleset-Cutscenes.md) — `slideshow: musicId:`
- [`extraSounds:`](Ruleset-ExtraSounds.md) — sound effects (a separate system)
