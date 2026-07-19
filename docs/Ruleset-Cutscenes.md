# Ruleset: `cutscenes:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleVideo`](../src/Mod/RuleVideo.h) · **List key:** `type` · **Loader:**
[`RuleVideo::load`](../src/Mod/RuleVideo.cpp)

A cutscene is either a list of **video files** (FLI/FLC/DOS movies) or a **slideshow** of still images
with captions. If both are defined, exactly **one** plays — the player's "preferred video" option
picks FMV or slideshow. Cutscenes are triggered by name from
[`research:`](Ruleset-Research.md), [`alienDeployments:`](Ruleset-AlienDeployments.md) and
[`events:`](Ruleset-Events.md) via their `cutscene:` fields, and by the engine for the intro.

```yaml
cutscenes:
  - type: intro
    useUfoAudioSequence: true
    videos:
      - UFOINTRO/UFOINT.FLI

  - type: winGame
    winGame: true
    slideshow:
      transitionSeconds: 30
      musicId: GMWIN
      slides:
        - imagePath: UFOINTRO/PICT1.LBM
          caption: STR_VICTORY_1
          captionSize: [195, 56]
          captionPos: [5, 0]
          captionColor: 249
```

Entries **merge** by `type`. The ids `winGame` and `loseGame` are recognized by name for backwards
compatibility (they set the corresponding flag automatically).

## Cutscene fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; this is the name other rules put in their `cutscene:` fields. |
| `videos` | list of paths | — | Video files to play in order (relative to the mod, e.g. `UFOINTRO/UFOINT.FLI`). |
| `audioTracks` | list of paths | — | Audio files played alongside the videos, one per video. |
| `useUfoAudioSequence` | bool | false | Use the hardcoded vanilla UFO-intro audio/timing sequence; in practice only correct for the original intro movie. |
| `winGame` | bool | `type == "winGame"` | Playing this cutscene **wins the campaign** (the game ends in victory after it). |
| `loseGame` | bool | `type == "loseGame"` | Playing this cutscene **loses the campaign**. |
| `slideshow` | map | — | A still-image slideshow, the alternative to `videos:` (never both) — see below. |

## `slideshow:`

| Key | Type | Default | Meaning |
|---|---|---|---|
| `musicId` | string | — | Music track to play for the whole slideshow (extension-less filename, e.g. `GMWIN`). |
| `transitionSeconds` | int | 30 | Default seconds each slide is shown before advancing. |
| `slides` | list | — | The slides, in order. |

### Slide fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `imagePath` | string | `""` | The slide image (e.g. `UFOINTRO/PICT1.LBM`). |
| `caption` | string | `""` | `STR_*` key of the caption text drawn over the slide. |
| `captionPos` | `[x, y]` | `[0, 0]` | Top-left corner of the caption box, in screen pixels. |
| `captionSize` | `[w, h]` | `[320, 200]` | Size of the caption box (defaults to the full original screen). |
| `captionColor` | int | palette default | Palette index the caption text is drawn in. |
| `captionAlign` | int | 0 | Horizontal caption alignment: 0 = left, 1 = center, 2 = right. |
| `captionVerticalAlign` | int | 0 | Vertical caption alignment: 0 = top, 1 = middle, 2 = bottom. |
| `transitionSeconds` | int | 0 | Seconds to show **this** slide; 0 means use the slideshow's `transitionSeconds`. |

## Notes

- A cutscene with **neither** `videos:` nor `slideshow:` still works as a trigger — it just plays
  nothing, which is the usual way to make a research topic "end the game" via `winGame: true` without
  showing a movie.
- Videos are only played if the player has the video files and `Options::playIntro`-style playback is
  available; a missing video falls through to the slideshow (if any) rather than crashing.
- Slide captions are localized like everything else — define the keys in
  [`extraStrings:`](Ruleset-ExtraStrings.md).

## See also

- [`musics:`](Ruleset-Musics.md) — the `musicId:` track
- [`research:`](Ruleset-Research.md) / [`alienDeployments:`](Ruleset-AlienDeployments.md) / [`events:`](Ruleset-Events.md) — what triggers a cutscene
- [`extraStrings:`](Ruleset-ExtraStrings.md) — caption text
