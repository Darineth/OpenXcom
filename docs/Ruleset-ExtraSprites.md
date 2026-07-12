# Ruleset: `extraSprites:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`ExtraSprites`](../src/Mod/ExtraSprites.h) · **List key:** `type` · **Loader:**
[`ExtraSprites::load`](../src/Mod/ExtraSprites.cpp)

`extraSprites:` is how a mod gets **images into the engine**. Every entry either adds/replaces frames
in a named **surface set** (the PCK/SPK sprite sheets the engine indexes by frame number, e.g.
`BIGOBS.PCK`, `FLOOROB.PCK`, `HANDOB.PCK`), or defines a **single standalone image** (a full-screen
background, a UI graphic) that rules refer to by name.

```yaml
extraSprites:
  - type: BIGOBS.PCK              # existing surface set: replace/add frames by index
    files:
      57: Resources/MyMod/bigobs_myrifle.png
      58: Resources/MyMod/bigobs/       # a folder: files load in natural sort order from 58 up

  - type: oxceLinks               # a standalone image
    width: 32
    height: 16
    singleImage: true
    files:
      0: Resources/Extended/oxceLinks.png

  - type: TinyRanks               # one PNG sliced into a set of 7x7 frames
    width: 42
    height: 7
    subX: 7
    subY: 7
    files:
      0: Resources/Extended/TinyRanks.png
```

Entries **accumulate** — several mods may add to the same `type`. `delete:` drops every entry for a
type. Accepted image formats: PNG, GIF, BMP, LBM, IFF, PCX, TGA, TIF/TIFF.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Name of the surface set (or of the single image) to create/patch. |
| `typeSingle` | string | — | Shorthand: sets `type` **and** implies `singleImage: true`. |
| `files` | map int → path | — | Frame index → image file. A path ending in `/` is a **folder**: its images load in natural sort order starting at that index. |
| `fileSingle` | string path | — | Shorthand for `files: { 0: <path> }`; only read when `type` is absent (i.e. alongside `typeSingle`). |
| `width` | int | 320 | Width of the source image / of each frame in a new set. |
| `height` | int | 200 | Height of the source image / of each frame in a new set. |
| `singleImage` | bool | false | Treat this as one standalone `Surface` rather than a frame set. |
| `subX` | int | 0 | Slice each source image into frames this many pixels **wide** (0 = no slicing). |
| `subY` | int | 0 | Slice each source image into frames this many pixels **tall** (0 = no slicing). |

## How the pieces interact

- **Frame set (default).** Each `files:` entry loads at that frame index. Existing frames are
  cleared and replaced; new indices are appended. Frames **at or above the set's shared-frame count**
  (the vanilla frame count) are automatically shifted into the **mod's own index range** — this is
  the mod-offset mechanism that keeps two mods from colliding, and it is why a rule referring to
  "your" sprite 57 still works after another mod loads. Exceeding the mod's index budget is a load
  error (`… exceeds mod '<name>' size limit`).
- **Subdivision.** With both `subX` and `subY` set, each file is loaded as a `width`×`height` image
  and cut into `(width/subX) × (height/subY)` frames, filling left-to-right then top-to-bottom from
  the starting index. This is the usual way to ship a spritesheet as one PNG.
- **Single image.** With `singleImage: true` (or `typeSingle:`) only the first file is used and the
  result is a plain `Surface` of `width`×`height` registered under `type`. Rules and interfaces that
  take an image name (backgrounds, `image_id`, `backgroundImage`…) refer to it by that name.
- **New sets.** If `type` names a set that does not exist yet, the set is created with the frame
  size given by `subX`/`subY` (if slicing) or `width`/`height` (if not).
- Sprites are loaded lazily when `lazyLoadResources` is on; the palette is applied by whatever screen
  uses the surface, so source images should use the target palette's colors.

## See also

- [`extraSounds:`](Ruleset-ExtraSounds.md) — the audio equivalent, with the same index/offset rules
- [`customPalettes:`](Ruleset-CustomPalettes.md) — palettes the sprites are drawn in
- [`interfaces:`](Ruleset-Interfaces.md) — `backgroundImage:` names single images defined here
- [`armors:`](Ruleset-Armors.md) — `spriteSheet:` / `spriteInv:` name sets defined here
